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

    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyReadStandardOutput,
            this, &MainWindow::readProcessOutput);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::processFinished);

    proc->start(program, args);
    if (!proc->waitForStarted(3000)) {
        appendLog("启动失败: " + proc->errorString(), tag);
        delete proc;
        return;
    }

    m_taskLabels.insert(proc, tag);

    // 登记任务到下拉列表（保持"全部"为首项）。
    TaskMeta meta{id, tag, shortInput, true};
    m_taskMetas.append(meta);
    m_taskFilter->addItem(QString("#%1 %2").arg(id).arg(shortInput), tag);

    // 同步到下载列表。
    if (m_taskList) {
        auto *it = new QListWidgetItem(taskDisplay(meta), m_taskList);
        it->setData(Qt::UserRole, tag);
    }

    m_tasks.append(proc);
    updateTaskCount();
}

void MainWindow::stopDownload()
{
    if (m_tasks.isEmpty())
        return;
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
}

void MainWindow::updateTaskCount()
{
    const int n = m_tasks.size();
    m_taskCountLabel->setText(QString("正在下载任务数量: %1").arg(n));
    m_stopBtn->setEnabled(n > 0);
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
    if (!m.finished)
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
