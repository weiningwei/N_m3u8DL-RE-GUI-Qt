#include "mainwindow.h"
#include "stringlistwidget.h"
#include "options.h"

#include <QApplication>
#include <QActionGroup>
#include <QKeySequence>
#include <QStyleHints>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QColor>
#include <QTabWidget>
#include <QScrollArea>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QFontDatabase>
#include <QCloseEvent>
#include <QMenuBar>
#include <QScrollBar>
#include <QOverload>

// ------------------------------------------------------------------
// Execution
// ------------------------------------------------------------------

void MainWindow::startDownload()
{
    const QString program = resolvedExePath();
    if (program.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先设置 N_m3u8DL-RE 可执行文件路径。");
        return;
    }
    if (!QFileInfo::exists(program)) {
        QMessageBox::warning(this, "提示", "找不到可执行文件:\n" + program);
        return;
    }
    if (m_input->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入链接或文件 (input)。");
        return;
    }

    const QStringList args = buildArguments();

    const int id = ++m_nextTaskId;
    const QString input = m_input->text().trimmed();
    QString shortInput = input;
    if (shortInput.size() > 28)
        shortInput = shortInput.left(28) + "…";
    const QString tag = QString("[#%1 %2]").arg(id).arg(shortInput);
    appendLog("> " + previewCommand(program, args), tag);

    // 独立终端模式：在新窗口中启动，不经过队列系统
    if (m_terminalModeBox && m_terminalModeBox->isChecked()) {
        QStringList cmdArgs;
        cmdArgs << "/c" << "start" << "\"N_m3u8DL-RE ["
                << QString::number(id) << "]\"" << "/D"
                << QDir::toNativeSeparators(QFileInfo(program).absolutePath())
                << QDir::toNativeSeparators(program);
        cmdArgs << args;
        if (QProcess::startDetached("cmd.exe", cmdArgs))
            appendLog("已在独立终端中启动。", tag);
        else
            appendLog("独立终端启动失败。", tag);
        return;
    }

    // 在请求时捕获命令行快照，避免排队期间输入框变化导致参数漂移。
    PendingTask pt;
    pt.program = program;
    pt.args = args;
    pt.id = id;
    pt.tag = tag;
    pt.label = shortInput;
    pt.meta = TaskMeta{id, tag, shortInput, input, true};
    pt.meta.waiting = true;

    // 登记任务到下拉列表（保持"全部"为首项）与下载列表（先显示"等待中"）。
    m_taskMetas.append(pt.meta);
    m_taskFilter->addItem(QString("#%1 %2").arg(id).arg(shortInput), tag);
    if (m_taskList) {
        auto *it = new QListWidgetItem(taskDisplay(pt.meta), m_taskList);
        it->setData(Qt::UserRole, tag);
    }

    // 未启用队列限制，或当前运行数尚未达到上限 -> 立即开始；否则入队等待。
    const bool limited = m_queueEnabledBox && m_queueEnabledBox->isChecked();
    const int limit = m_maxConcurrentBox ? m_maxConcurrentBox->value() : 0;
    if (!limited || limit <= 0 || m_tasks.size() < limit)
        startTaskNow(pt);
    else
        enqueueTask(pt);
}

void MainWindow::startTaskNow(const PendingTask &pt)
{
    // 标记不再等待，并刷新下载列表项文案为"运行中"。
    for (TaskMeta &m : m_taskMetas) {
        if (m.tag == pt.tag) {
            m.waiting = false;
            setTaskListText(pt.tag, taskDisplay(m));
            break;
        }
    }

    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyReadStandardOutput,
            this, &MainWindow::readProcessOutput);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::processFinished);

    proc->start(pt.program, pt.args);
    if (!proc->waitForStarted(3000)) {
        appendLog("启动失败: " + proc->errorString(), pt.tag);
        delete proc;
        return;
    }

    m_taskLabels.insert(proc, pt.tag);
    m_tasks.append(proc);
    updateTaskCount();
}

void MainWindow::enqueueTask(const PendingTask &pt)
{
    m_waiting.append(pt);
    appendLog(QString("已加入等待队列（当前等待 %1 个）。").arg(m_waiting.size()),
              pt.tag);
    updateTaskCount();
}

void MainWindow::maybeStartQueued()
{
    while (!m_waiting.isEmpty()) {
        if (m_queueEnabledBox && m_queueEnabledBox->isChecked()) {
            const int limit = m_maxConcurrentBox ? m_maxConcurrentBox->value() : 0;
            if (limit <= 0 || m_tasks.size() >= limit)
                break;
        }
        startTaskNow(m_waiting.takeFirst());
    }
    updateTaskCount();
}

void MainWindow::setTaskListText(const QString &tag, const QString &text)
{
    if (!m_taskList)
        return;
    for (int i = 0; i < m_taskList->count(); ++i) {
        auto *it = m_taskList->item(i);
        if (it->data(Qt::UserRole).toString() == tag) {
            it->setText(text);
            break;
        }
    }
}

void MainWindow::removeTaskByTag(const QString &tag)
{
    for (int i = m_taskMetas.size() - 1; i >= 0; --i)
        if (m_taskMetas.at(i).tag == tag)
            m_taskMetas.removeAt(i);
    const int idx = m_taskFilter ? m_taskFilter->findData(tag) : -1;
    if (idx >= 0)
        m_taskFilter->removeItem(idx);
    if (m_taskList) {
        for (int i = m_taskList->count() - 1; i >= 0; --i) {
            if (m_taskList->item(i)->data(Qt::UserRole).toString() == tag) {
                delete m_taskList->takeItem(i);
                break;
            }
        }
    }
}

void MainWindow::onQueueToggled(bool on)
{
    if (m_maxConcurrentBox)
        m_maxConcurrentBox->setEnabled(on);
    auto s = portableSettings();
    s.setValue("queueEnabled", on);
    // 关闭限制 -> 立即放开所有等待任务（maybeStartQueued 在禁用时不受上限约束）。
    if (!on)
        maybeStartQueued();
    updateTaskCount();
}

void MainWindow::onMaxConcurrentChanged(int v)
{
    auto s = portableSettings();
    s.setValue("maxConcurrent", v);
    // 调大上限后若有空位，立即填补等待队列。
    maybeStartQueued();
}

void MainWindow::stopDownload()
{
    // 先清空等待队列，避免随后 processFinished 的自动续传逻辑把等待任务重新拉起。
    if (!m_waiting.isEmpty()) {
        for (const PendingTask &pt : m_waiting)
            removeTaskByTag(pt.tag);
        m_waiting.clear();
        appendLog("已清空等待队列。");
    }
    if (m_tasks.isEmpty()) {
        updateTaskCount();
        return;
    }
    const int n = m_tasks.size();
    for (QProcess *proc : m_tasks) {
        if (proc->state() != QProcess::NotRunning)
            proc->kill();
    }
    appendLog(QString("已发送停止信号 (%1 个任务)。").arg(n));
}

void MainWindow::readProcessOutput()
{
    auto *proc = qobject_cast<QProcess *>(sender());
    if (!proc)
        return;
    const QString tag = m_taskLabels.value(proc);
    const QByteArray data = proc->readAll();

    // 按行加前缀，保证每条日志都能区分所属任务。
    QString &buf = m_partial[proc];
    buf += QString::fromUtf8(data);
    QStringList lines = buf.split('\n');
    buf = lines.takeLast(); // 末尾可能是不完整的半行，留待下次拼接
    for (const QString &line : lines)
        appendLog(line, tag);

    // Keep the view pinned to the newest output.
    QScrollBar *bar = m_log->verticalScrollBar();
    if (bar)
        bar->setValue(bar->maximum());
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus status)
{
    auto *proc = qobject_cast<QProcess *>(sender());
    const QString tag = proc ? m_taskLabels.value(proc) : QString();

    // 冲刷该任务残留的半行日志。
    if (proc && m_partial.contains(proc) && !m_partial[proc].isEmpty())
        appendLog(m_partial.take(proc), tag);

    const QString msg = QString("%1 进程结束 (退出码 %2, %3)")
                            .arg(tag)
                            .arg(exitCode)
                            .arg(status == QProcess::NormalExit ? "正常退出" : "异常退出");
    appendLog(msg);

    if (proc) {
        m_tasks.removeAll(proc);
        m_taskLabels.remove(proc);
        m_partial.remove(proc);
        // 标记为已完成并更新下拉项文案（保留历史任务可继续过滤查看）。
        for (TaskMeta &m : m_taskMetas) {
            if (m.tag == tag) {
                m.active = false;
                m.finished = true;
                m.success = (status == QProcess::NormalExit && exitCode == 0);
                m.exitCode = exitCode;
                const int idx = m_taskFilter->findData(tag);
                if (idx >= 0)
                    m_taskFilter->setItemText(idx, QString("#%1 %2 (已完成)").arg(m.id).arg(m.label));
                // 更新下载列表项文案与颜色。
                if (m_taskList) {
                    for (int i = 0; i < m_taskList->count(); ++i) {
                        auto *it = m_taskList->item(i);
                        if (it->data(Qt::UserRole).toString() == tag) {
                            it->setText(taskDisplay(m));
                            it->setForeground(m.success
                                                  ? QColor(46, 125, 50)
                                                  : QColor(192, 64, 40));
                            break;
                        }
                    }
                }
                break;
            }
        }
        proc->deleteLater();
    }
    updateTaskCount();
    // 有任务结束 -> 若等待队列中有任务且仍有空位，自动开启下一个。
    maybeStartQueued();
}

void MainWindow::updateTaskCount()
{
    const int n = m_tasks.size();
    const int w = m_waiting.size();
    QString text = QString("运行中: %1").arg(n);
    if (w > 0)
        text += QString("  等待中: %1").arg(w);
    m_taskCountLabel->setText(text);
    m_stopBtn->setEnabled(n > 0 || w > 0);
}

void MainWindow::appendLog(const QString &text, const QString &tag)
{
    m_logLines.append({tag, text});
    // 当前显示"全部"或正筛选该任务时，直接追加；否则仅记录、切换筛选时再重绘。
    if (m_taskFilter == nullptr || m_taskFilter->currentIndex() <= 0
            || tag == currentFilterTag()) {
        m_log->appendPlainText((tag.isEmpty() ? QString() : (tag + " ")) + text);
        QScrollBar *bar = m_log->verticalScrollBar();
        if (bar)
            bar->setValue(bar->maximum());
    }
}

QString MainWindow::currentFilterTag() const
{
    if (!m_taskFilter || m_taskFilter->currentIndex() <= 0)
        return QString();
    return m_taskFilter->currentData().toString();
}

void MainWindow::refreshLogView()
{
    const QString sel = currentFilterTag();
    QString out;
    for (const LogLine &l : m_logLines) {
        if (!sel.isEmpty() && l.tag != sel)
            continue;
        out += (l.tag.isEmpty() ? QString() : (l.tag + " ")) + l.text + "\n";
    }
    m_log->setPlainText(out);
    QScrollBar *bar = m_log->verticalScrollBar();
    if (bar)
        bar->setValue(bar->maximum());
}

void MainWindow::onTaskFilterChanged(int /*index*/)
{
    refreshLogView();
}

QString MainWindow::taskDisplay(const TaskMeta &m) const
{
    QString s = QString("#%1 %2").arg(m.id).arg(m.label);
    if (m.waiting)
        s += " — 等待中";
    else if (!m.finished)
        s += " — 运行中";
    else
        s += m.success ? " — 已完成"
                       : QString(" — 异常退出(%1)").arg(m.exitCode);
    return s;
}

void MainWindow::onTaskListClicked(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString tag = item->data(Qt::UserRole).toString();
    if (tag.isEmpty())
        m_taskFilter->setCurrentIndex(0);   // “全部”
    else {
        const int idx = m_taskFilter->findData(tag);
        if (idx >= 0)
            m_taskFilter->setCurrentIndex(idx);
    }
    // 触发 onTaskFilterChanged -> refreshLogView 按任务筛选日志
}

// ------------------------------------------------------------------
// 右键菜单
// ------------------------------------------------------------------

void MainWindow::stopTaskByTag(const QString &tag)
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        QProcess *proc = m_tasks.at(i);
        if (m_taskLabels.value(proc) == tag) {
            if (proc->state() != QProcess::NotRunning)
                proc->kill();
            appendLog("已停止任务。", tag);
            return;
        }
    }
}

void MainWindow::cancelWaitingByTag(const QString &tag)
{
    int idx = -1;
    for (int i = 0; i < m_waiting.size(); ++i) {
        if (m_waiting.at(i).tag == tag) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return;
    m_waiting.removeAt(idx);
    removeTaskByTag(tag);
    appendLog("已取消等待。");
    updateTaskCount();
}

void MainWindow::clearLog()
{
    m_logLines.clear();
    m_log->clear();
    m_taskFilter->setCurrentIndex(0);  // 切回"全部"后日志为空
}

void MainWindow::copyAllLog()
{
    QString out;
    for (const LogLine &l : m_logLines)
        out += (l.tag.isEmpty() ? QString() : (l.tag + " ")) + l.text + "\n";
    QApplication::clipboard()->setText(out);
}

void MainWindow::copyVisibleLog()
{
    const QString sel = currentFilterTag();
    QString out;
    for (const LogLine &l : m_logLines) {
        if (!sel.isEmpty() && l.tag != sel)
            continue;
        out += (l.tag.isEmpty() ? QString() : (l.tag + " ")) + l.text + "\n";
    }
    QApplication::clipboard()->setText(out);
}

void MainWindow::onTaskListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_taskList->itemAt(pos);
    if (!item)
        return;
    const QString tag = item->data(Qt::UserRole).toString();
    if (tag.isEmpty())
        return; // "全部"

    TaskMeta *meta = nullptr;
    for (TaskMeta &m : m_taskMetas) {
        if (m.tag == tag) {
            meta = &m;
            break;
        }
    }
    if (!meta)
        return;

    QMenu menu;
    if (meta->waiting) {
        menu.addAction("取消等待", this, [this, tag]() { cancelWaitingByTag(tag); });
    } else if (!meta->finished) {
        menu.addAction("停止此任务", this, [this, tag]() { stopTaskByTag(tag); });
    }
    menu.addSeparator();
    if (!meta->input.isEmpty())
        menu.addAction("复制链接", [meta]() { QApplication::clipboard()->setText(meta->input); });
    menu.addAction("复制标签", [tag]() { QApplication::clipboard()->setText(tag); });
    menu.exec(m_taskList->viewport()->mapToGlobal(pos));
}

void MainWindow::onLogContextMenu(const QPoint &pos)
{
    QMenu *menu = m_log->createStandardContextMenu();
    menu->addSeparator();
    menu->addAction("清除日志", this, [this]() { clearLog(); });
    menu->addAction("复制全部日志", this, [this]() { copyAllLog(); });
    menu->addAction("复制可见日志", this, [this]() { copyVisibleLog(); });
    menu->exec(m_log->viewport()->mapToGlobal(pos));
    delete menu;
}
