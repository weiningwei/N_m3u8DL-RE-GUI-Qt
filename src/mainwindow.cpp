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
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QGuiApplication>
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("N_m3u8DL-RE GUI (Qt)");
    resize(920, 720);

    auto *central = new QWidget(this);
    m_root = new QVBoxLayout(central);
    m_root->setSpacing(6);
    setCentralWidget(central);

    buildToolbar();
    buildInputArea();
    buildOptionsUi();
    buildRunArea();

    loadSettings();
    autoDetectExe();
    autoDetectFfmpeg();
    generatePreview();

    connect(m_input, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
    connect(m_exePath, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
}

// ------------------------------------------------------------------
// UI construction
// ------------------------------------------------------------------

void MainWindow::buildToolbar()
{
    auto *fileMenu = menuBar()->addMenu("文件");
    QAction *exitAction = fileMenu->addAction("退出", this, &MainWindow::requestExit);
    exitAction->setShortcut(QKeySequence(Qt::Key_Escape));

    auto *settingsMenu = menuBar()->addMenu("设置");

    auto *themeMenu = settingsMenu->addMenu("主题");
    for (const QString &name : {QStringLiteral("跟随系统"),
                                QStringLiteral("浅色"),
                                QStringLiteral("暗色")}) {
        QAction *a = themeMenu->addAction(name);
        a->setCheckable(true);
        m_themeActions.append(a);
        connect(a, &QAction::triggered,
                this, [this, a]() { onThemeActionTriggered(a); });
    }

    m_alwaysOnTopAction = settingsMenu->addAction("窗口置顶");
    m_alwaysOnTopAction->setCheckable(true);
    connect(m_alwaysOnTopAction, &QAction::toggled,
            this, &MainWindow::onAlwaysOnTopToggled);

    auto *resetAction = new QAction("恢复默认配置");
    connect(resetAction, &QAction::triggered, this, &MainWindow::resetSettings);
    settingsMenu->addAction(resetAction);

    auto *helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction("关于", this, &MainWindow::about);

    // 主页面工具栏：直接提供主题切换与窗口置顶，无需进入二级菜单。
    auto *toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);

    auto *themeBtn = new QToolButton(toolBar);
    themeBtn->setText("主题");
    themeBtn->setPopupMode(QToolButton::InstantPopup);
    auto *themePopup = new QMenu(themeBtn);
    for (QAction *a : m_themeActions)
        themePopup->addAction(a);
    themeBtn->setMenu(themePopup);
    toolBar->addWidget(themeBtn);

    toolBar->addSeparator();
    toolBar->addAction(m_alwaysOnTopAction);
    toolBar->addAction(resetAction);
}

void MainWindow::buildInputArea()
{
    auto *exeRow = new QHBoxLayout();
    exeRow->addWidget(new QLabel("程序路径:"));
    m_exePath = new QLineEdit();
    m_exePath->setPlaceholderText("N_m3u8DL-RE 可执行文件 (如 N_m3u8DL-RE.exe)");
    exeRow->addWidget(m_exePath, 1);
    auto *exeBtn = new QPushButton("浏览...");
    connect(exeBtn, &QPushButton::clicked, this, &MainWindow::browseExe);
    exeRow->addWidget(exeBtn);
    m_root->addLayout(exeRow);

    auto *ffRow = new QHBoxLayout();
    ffRow->addWidget(new QLabel("ffmpeg路径:"));
    m_ffmpegPath = new QLineEdit();
    m_ffmpegPath->setPlaceholderText("ffmpeg 可执行文件 (可选, 留空则自动查找)");
    ffRow->addWidget(m_ffmpegPath, 1);
    auto *ffBtn = new QPushButton("浏览...");
    connect(ffBtn, &QPushButton::clicked,
            this, [this]() { browseFor(m_ffmpegPath, false); });
    ffRow->addWidget(ffBtn);
    m_root->addLayout(ffRow);

    connect(m_exePath, &QLineEdit::textChanged,
            this, [this]() { onPathTextChanged(m_exePath); });
    connect(m_ffmpegPath, &QLineEdit::textChanged,
            this, [this]() { onPathTextChanged(m_ffmpegPath); });

    auto *inRow = new QHBoxLayout();
    inRow->addWidget(new QLabel("链接/文件:"));
    m_input = new QLineEdit();
    m_input->setPlaceholderText("输入 m3u8/dash 链接或本地文件 (input)");
    inRow->addWidget(m_input, 1);
    m_root->addLayout(inRow);
    m_root->addSpacing(4);
}

QWidget *MainWindow::makeOptionWidget(const Opt &opt, QWidget **valueWidget)
{
    switch (opt.type) {
    case Opt::Bool: {
        auto *cb = new QCheckBox();
        cb->setChecked(opt.boolDefault);
        cb->setToolTip(opt.tooltip);
        *valueWidget = cb;
        return cb;
    }
    case Opt::String:
    case Opt::PathFile:
    case Opt::PathDir: {
        auto *le = new QLineEdit();
        le->setToolTip(opt.tooltip);
        le->setPlaceholderText(opt.def);
        if (opt.type == Opt::String) {
            *valueWidget = le;
            return le;
        }
        auto *container = new QWidget();
        auto *hl = new QHBoxLayout(container);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->addWidget(le, 1);
        auto *btn = new QPushButton("浏览");
        hl->addWidget(btn);
        const bool dirOnly = (opt.type == Opt::PathDir);
        connect(btn, &QPushButton::clicked, this,
                [this, le, dirOnly]() { browseFor(le, dirOnly); });
        *valueWidget = le;
        return container;
    }
    case Opt::Int: {
        auto *sp = new QSpinBox();
        sp->setRange(0, 999999);
        sp->setValue(opt.intDefault);
        sp->setToolTip(opt.tooltip);
        *valueWidget = sp;
        return sp;
    }
    case Opt::Combo: {
        auto *cb = new QComboBox();
        cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        cb->addItems(opt.choices);
        const int idx = opt.choices.indexOf(opt.def);
        if (idx >= 0)
            cb->setCurrentIndex(idx);
        cb->setToolTip(opt.tooltip);
        *valueWidget = cb;
        return cb;
    }
    case Opt::List: {
        auto *lw = new StringListWidget();
        lw->setToolTip(opt.tooltip);
        *valueWidget = lw;
        return lw;
    }
    }
    *valueWidget = nullptr;
    return nullptr;
}

void MainWindow::buildOptionsUi()
{
    m_tabs = new QTabWidget(this);
    const QVector<Category> cats = buildCategories();
    for (const Category &cat : cats) {
        auto *page = new QWidget(m_tabs);
        auto *form = new QFormLayout(page);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignLeft);
        form->setHorizontalSpacing(10);

        for (const Opt &o : cat.opts) {
            QWidget *valueWidget = nullptr;
            QWidget *field = makeOptionWidget(o, &valueWidget);
            auto *label = new QLabel(o.label, page);
            label->setToolTip(o.tooltip);
            form->addRow(label, field);

            Entry e{o, valueWidget};
            m_entries.append(e);
            connectEntryPreview(e);
        }
        m_tabs->addTab(page, cat.name);
    }
    m_root->addWidget(m_tabs, 1);
}

void MainWindow::buildRunArea()
{
    auto *prevRow = new QHBoxLayout();
    prevRow->addWidget(new QLabel("命令预览:"));
    m_cmdPreview = new QLineEdit();
    m_cmdPreview->setReadOnly(true);
    prevRow->addWidget(m_cmdPreview, 1);
    auto *copyBtn = new QPushButton("复制");
    connect(copyBtn, &QPushButton::clicked, this, &MainWindow::copyCommand);
    prevRow->addWidget(copyBtn);
    m_root->addLayout(prevRow);

    auto *btnRow = new QHBoxLayout();
    m_startBtn = new QPushButton("开始下载");
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setEnabled(false);
    auto *genBtn = new QPushButton("生成命令");
    auto *openBtn = new QPushButton("打开输出目录");
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::startDownload);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::stopDownload);
    connect(genBtn, &QPushButton::clicked, this, &MainWindow::generatePreview);
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::openOutputDir);
    btnRow->addWidget(m_startBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(genBtn);
    btnRow->addWidget(openBtn);
    m_taskCountLabel = new QLabel("正在下载任务数量: 0");
    btnRow->addWidget(m_taskCountLabel);
    btnRow->addStretch(1);
    m_root->addLayout(btnRow);

    auto *filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("日志过滤:"));
    m_taskFilter = new QComboBox();
    m_taskFilter->addItem("全部");
    m_taskFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    filterRow->addWidget(m_taskFilter, 1);
    m_root->addLayout(filterRow);
    connect(m_taskFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTaskFilterChanged);

    m_log = new QPlainTextEdit();
    m_log->setReadOnly(true);
    m_log->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_root->addWidget(m_log, 2);
}

void MainWindow::connectEntryPreview(const Entry &e)
{
    switch (e.opt.type) {
    case Opt::Bool:
        connect(qobject_cast<QCheckBox *>(e.widget), &QCheckBox::toggled,
                this, &MainWindow::generatePreview);
        break;
    case Opt::String:
    case Opt::PathFile:
    case Opt::PathDir:
        connect(qobject_cast<QLineEdit *>(e.widget), &QLineEdit::textChanged,
                this, &MainWindow::generatePreview);
        break;
    case Opt::Int:
        connect(qobject_cast<QSpinBox *>(e.widget),
                QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this]() { generatePreview(); });
        break;
    case Opt::Combo:
        connect(qobject_cast<QComboBox *>(e.widget), &QComboBox::currentTextChanged,
                this, &MainWindow::generatePreview);
        break;
    case Opt::List:
        connect(qobject_cast<StringListWidget *>(e.widget), &StringListWidget::itemsChanged,
                this, &MainWindow::generatePreview);
        break;
    }
}

// ------------------------------------------------------------------
// Browsing helpers
// ------------------------------------------------------------------

void MainWindow::browseExe()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "选择 N_m3u8DL-RE 可执行文件", QString(),
        "可执行文件 (*.exe);;所有文件 (*.*)");
    if (!path.isEmpty())
        m_exePath->setText(QDir::toNativeSeparators(path));
}

void MainWindow::browseFor(QLineEdit *target, bool dirOnly)
{
    if (dirOnly) {
        const QString dir = QFileDialog::getExistingDirectory(
            this, "选择目录", target->text());
        if (!dir.isEmpty())
            target->setText(QDir::toNativeSeparators(dir));
    } else {
        const QString file = QFileDialog::getOpenFileName(
            this, "选择文件", target->text());
        if (!file.isEmpty())
            target->setText(QDir::toNativeSeparators(file));
    }
}

// ------------------------------------------------------------------
// Command building & preview
// ------------------------------------------------------------------

QStringList MainWindow::buildArguments()
{
    QStringList args;
    const QString input = m_input->text().trimmed();
    if (!input.isEmpty())
        args.append(input);

    for (const Entry &e : m_entries) {
        const Opt &o = e.opt;
        switch (o.type) {
        case Opt::Bool: {
            auto *cb = qobject_cast<QCheckBox *>(e.widget);
            if (cb && cb->isChecked())
                args.append(o.flag);
            break;
        }
        case Opt::String:
        case Opt::PathFile:
        case Opt::PathDir: {
            auto *le = qobject_cast<QLineEdit *>(e.widget);
            QString v = le ? le->text().trimmed() : QString();
            if (v.isEmpty() && !o.defaultSubdir.isEmpty())
                v = QDir(QApplication::applicationDirPath()).filePath(o.defaultSubdir);
            if (!v.isEmpty()) {
                args.append(o.flag);
                args.append(v);
            }
            break;
        }
        case Opt::Int: {
            auto *sp = qobject_cast<QSpinBox *>(e.widget);
            const int v = sp ? sp->value() : o.intDefault;
            if (v != o.intDefault || o.emitDefault) {
                args.append(o.flag);
                args.append(QString::number(v));
            }
            break;
        }
        case Opt::Combo: {
            auto *cb = qobject_cast<QComboBox *>(e.widget);
            const QString v = cb ? cb->currentText() : QString();
            if (!v.isEmpty() && v != o.def) {
                args.append(o.flag);
                args.append(v);
            }
            break;
        }
        case Opt::List: {
            auto *lw = qobject_cast<StringListWidget *>(e.widget);
            if (lw) {
                for (const QString &it : lw->items()) {
                    const QString t = it.trimmed();
                    if (!t.isEmpty()) {
                        args.append(o.flag);
                        args.append(t);
                    }
                }
            }
            break;
        }
        }
    }

    // ffmpeg 路径（顶层选择器，对应 --ffmpeg-binary-path）
    const QString ffmpeg = resolvedFfmpegPath();
    if (!ffmpeg.isEmpty()) {
        args.append("--ffmpeg-binary-path");
        args.append(ffmpeg);
    }
    return args;
}

QString MainWindow::previewCommand(const QString &program, const QStringList &args)
{
    auto quote = [](const QString &s) -> QString {
        if (s.isEmpty())
            return "\"\"";
        const bool need = s.contains(' ') || s.contains('"') || s.contains('&')
                          || s.contains('(') || s.contains(')') || s.contains('^')
                          || s.contains('%');
        if (!need)
            return s;
        QString out = s;
        out.replace('"', "\\\"");
        return "\"" + out + "\"";
    };

    QStringList parts;
    if (!program.isEmpty())
        parts.append(quote(program));
    for (const QString &a : args)
        parts.append(quote(a));
    return parts.join(" ");
}

void MainWindow::generatePreview()
{
    const QString program = resolvedExePath();
    const QStringList args = buildArguments();
    m_cmdPreview->setText(previewCommand(program, args));
}

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
                const int idx = m_taskFilter->findData(tag);
                if (idx >= 0)
                    m_taskFilter->setItemText(idx, QString("#%1 %2 (已完成)").arg(m.id).arg(m.label));
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

// ------------------------------------------------------------------
// Misc actions
// ------------------------------------------------------------------

void MainWindow::copyCommand()
{
    QGuiApplication::clipboard()->setText(m_cmdPreview->text());
    appendLog("命令已复制到剪贴板。");
}

void MainWindow::openOutputDir()
{
    QString dir;
    for (const Entry &e : m_entries) {
        if (e.opt.flag == "--save-dir") {
            dir = qobject_cast<QLineEdit *>(e.widget)->text().trimmed();
            break;
        }
    }
    if (dir.isEmpty())
        dir = QDir::currentPath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::resetSettings()
{
    const QMessageBox::StandardButton r = QMessageBox::question(
        this, "恢复默认配置",
        "确定要清空所有已保存的设置并恢复默认配置吗？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r != QMessageBox::Yes)
        return;

    QSettings().clear();
    resetWidgetsToDefaults();
    autoDetectExe();
    autoDetectFfmpeg();
    generatePreview();
    QSettings().sync();
}

void MainWindow::resetWidgetsToDefaults()
{
    for (const Entry &e : m_entries) {
        switch (e.opt.type) {
        case Opt::Bool:
            qobject_cast<QCheckBox *>(e.widget)->setChecked(e.opt.boolDefault);
            break;
        case Opt::String:
        case Opt::PathFile:
        case Opt::PathDir:
            qobject_cast<QLineEdit *>(e.widget)->clear();
            break;
        case Opt::Int:
            qobject_cast<QSpinBox *>(e.widget)->setValue(e.opt.intDefault);
            break;
        case Opt::Combo: {
            auto *cb = qobject_cast<QComboBox *>(e.widget);
            const int i = cb->findText(e.opt.def);
            cb->setCurrentIndex(i >= 0 ? i : 0);
            break;
        }
        case Opt::List:
            qobject_cast<StringListWidget *>(e.widget)->clear();
            break;
        }
    }

    m_exePath->clear();
    m_ffmpegPath->clear();
    setTheme("跟随系统");
    m_alwaysOnTopAction->blockSignals(true);
    m_alwaysOnTopAction->setChecked(false);
    m_alwaysOnTopAction->blockSignals(false);
    setWindowFlag(Qt::WindowStaysOnTopHint, false);
}

void MainWindow::about()
{
    QMessageBox::about(this, "关于",
        "N_m3u8DL-RE GUI (Qt)\n\n"
        "基于 Qt 的 N_m3u8DL-RE 图形界面。\n"
        "版本 1.0.0");
}

void MainWindow::requestExit()
{
    if (!m_tasks.isEmpty()) {
        const QMessageBox::StandardButton r = QMessageBox::question(
            this, "确认退出",
            QString("当前有 %1 个下载任务正在进行，确定要退出吗？").arg(m_tasks.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (r != QMessageBox::Yes)
            return;
    }
    close();
}

void MainWindow::onThemeActionTriggered(QAction *action)
{
    QString target = action->text();
    // 再次点击已选中的项 -> 取消，回到"跟随系统"。
    if (target == m_currentTheme)
        target = "跟随系统";
    setTheme(target);
}

void MainWindow::setTheme(const QString &name)
{
    m_currentTheme = name;

    Qt::ColorScheme scheme = Qt::ColorScheme::Unknown; // 跟随系统
    if (name == "浅色")
        scheme = Qt::ColorScheme::Light;
    else if (name == "暗色")
        scheme = Qt::ColorScheme::Dark;
    QApplication::styleHints()->setColorScheme(scheme);

    for (QAction *a : m_themeActions) {
        a->blockSignals(true);
        a->setChecked(a->text() == name);
        a->blockSignals(false);
    }

    QSettings s;
    s.setValue("theme", name);
}

void MainWindow::onAlwaysOnTopToggled(bool on)
{
    // 直接用 Win32 调整窗口 Z 序，避免在已显示窗口上调用 setWindowFlag
    // 触发原生窗口重建（表现为界面闪烁）。
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowPos(hwnd, on ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
    setWindowFlag(Qt::WindowStaysOnTopHint, on);
    show();
#endif

    QSettings s;
    s.setValue("alwaysOnTop", on);
}

// ------------------------------------------------------------------
// Persistence
// ------------------------------------------------------------------

void MainWindow::saveSettings()
{
    QSettings s;
    s.setValue("exePath", m_exePath->text());
    s.setValue("ffmpegPath", m_ffmpegPath->text());
    // 注意：链接/文件 与 保存文件名 不持久化
    for (const Entry &e : m_entries) {
        if (e.opt.flag == "--save-name")
            continue;
        const QString key = "opt/" + e.opt.flag;
        switch (e.opt.type) {
        case Opt::Bool:
            s.setValue(key, qobject_cast<QCheckBox *>(e.widget)->isChecked());
            break;
        case Opt::String:
        case Opt::PathFile:
        case Opt::PathDir:
            s.setValue(key, qobject_cast<QLineEdit *>(e.widget)->text());
            break;
        case Opt::Int:
            s.setValue(key, qobject_cast<QSpinBox *>(e.widget)->value());
            break;
        case Opt::Combo:
            s.setValue(key, qobject_cast<QComboBox *>(e.widget)->currentText());
            break;
        case Opt::List:
            s.setValue(key, qobject_cast<StringListWidget *>(e.widget)->items());
            break;
        }
    }
}

void MainWindow::loadSettings()
{
    QSettings s;
    m_exePath->setText(s.value("exePath", "").toString());
    m_ffmpegPath->setText(s.value("ffmpegPath", "").toString());
    // 注意：链接/文件 与 保存文件名 不持久化，故不在此恢复
    for (const Entry &e : m_entries) {
        if (e.opt.flag == "--save-name")
            continue;
        const QVariant v = s.value("opt/" + e.opt.flag);
        if (!v.isValid())
            continue;
        switch (e.opt.type) {
        case Opt::Bool:
            qobject_cast<QCheckBox *>(e.widget)->setChecked(v.toBool());
            break;
        case Opt::String:
        case Opt::PathFile:
        case Opt::PathDir:
            qobject_cast<QLineEdit *>(e.widget)->setText(v.toString());
            break;
        case Opt::Int:
            qobject_cast<QSpinBox *>(e.widget)->setValue(v.toInt());
            break;
        case Opt::Combo: {
            auto *cb = qobject_cast<QComboBox *>(e.widget);
            const int i = cb->findText(v.toString());
            if (i >= 0)
                cb->setCurrentIndex(i);
            break;
        }
        case Opt::List:
            qobject_cast<StringListWidget *>(e.widget)->setItems(v.toStringList());
            break;
        }
    }
    applyUiSettings();
}

void MainWindow::applyUiSettings()
{
    QSettings s;
    setTheme(s.value("theme", "跟随系统").toString());

    const bool onTop = s.value("alwaysOnTop", false).toBool();
    m_alwaysOnTopAction->blockSignals(true);
    m_alwaysOnTopAction->setChecked(onTop);
    m_alwaysOnTopAction->blockSignals(false);
    setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
}

QString MainWindow::findInStandardLocations(const QString &exeName) const
{
    const QString appDir = QApplication::applicationDirPath();
    QStringList candidates;
    // 1) GUI 程序所在目录
    candidates.append(QDir(appDir).filePath(exeName));
    // 2) GUI 程序所在目录 /third_party/
    candidates.append(QDir(appDir + "/third_party").filePath(exeName));
    // 3) 环境变量 PATH
    const QString pathEnv = QProcessEnvironment::systemEnvironment().value("PATH");
    for (const QString &dir : pathEnv.split(';', Qt::SkipEmptyParts))
        candidates.append(QDir(dir).filePath(exeName));

    for (const QString &c : candidates) {
        if (QFileInfo::exists(c))
            return QDir::toNativeSeparators(c);
    }
    return QString();
}

void MainWindow::refreshPathHint(QLineEdit *edit, const QString &exeName)
{
    const QString detected = findInStandardLocations(exeName);
    const QString detCanon = detected.isEmpty()
                                 ? QString()
                                 : QFileInfo(detected).canonicalFilePath();
    const bool matches = !detCanon.isEmpty() && effectiveAbsolute(edit) == detCanon;
    if (matches) {
        // 命中自动探测位置：记录绝对路径、显示相对路径、加绿色提示。
        edit->setProperty("absPath", detCanon);
        edit->setToolTip("已自动检测到，可手动修改");
        edit->setStyleSheet("QLineEdit { border: 2px solid #2e7d32; border-radius: 3px; }");
        const QString rel = relativeDisplay(detCanon);
        if (edit->text() != rel)
            edit->setText(rel);
    } else if (!edit->property("absPath").toString().isEmpty()) {
        // 原为自动检测，但内容已不再命中，取消提示。
        edit->setProperty("absPath", QString());
        edit->setProperty("autoPath", QString());
        edit->setToolTip(QString());
        edit->setStyleSheet(QString());
    }
}

void MainWindow::onPathTextChanged(QLineEdit *edit)
{
    // 当内容已不再是自动检测到的相对路径时，去掉绿色提示框。
    const QString abs = edit->property("absPath").toString();
    if (!abs.isEmpty() && edit->text() != relativeDisplay(abs)) {
        edit->setProperty("absPath", QString());
        edit->setProperty("autoPath", QString());
        edit->setToolTip(QString());
        edit->setStyleSheet(QString());
    }
}

QString MainWindow::relativeDisplay(const QString &absPath) const
{
    return QDir(QApplication::applicationDirPath()).relativeFilePath(absPath);
}

QString MainWindow::effectiveAbsolute(QLineEdit *edit) const
{
    QString eff = edit->property("absPath").toString();
    if (eff.isEmpty()) {
        const QString t = edit->text().trimmed();
        if (t.isEmpty())
            return QString();
        eff = QDir::isAbsolutePath(t)
                  ? t
                  : QDir(QApplication::applicationDirPath()).absoluteFilePath(t);
    }
    return QFileInfo(eff).canonicalFilePath();
}

QString MainWindow::resolvedExePath() const
{
    const QString abs = m_exePath->property("absPath").toString();
    return abs.isEmpty() ? m_exePath->text().trimmed() : abs;
}

QString MainWindow::resolvedFfmpegPath() const
{
    const QString abs = m_ffmpegPath->property("absPath").toString();
    return abs.isEmpty() ? m_ffmpegPath->text().trimmed() : abs;
}

void MainWindow::autoDetectExe()
{
    if (m_exePath->text().isEmpty()) {
        const QString found = findInStandardLocations("N_m3u8DL-RE.exe");
        if (!found.isEmpty())
            m_exePath->setText(found);
    }
    refreshPathHint(m_exePath, "N_m3u8DL-RE.exe");
}

void MainWindow::autoDetectFfmpeg()
{
    if (m_ffmpegPath->text().isEmpty()) {
        const QString found = findInStandardLocations("ffmpeg.exe");
        if (!found.isEmpty())
            m_ffmpegPath->setText(found);
    }
    refreshPathHint(m_ffmpegPath, "ffmpeg.exe");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const QVector<QProcess *> tasks = m_tasks;
    for (QProcess *proc : tasks) {
        if (proc->state() != QProcess::NotRunning) {
            proc->kill();
            proc->waitForFinished(2000);
        }
    }
    m_tasks.clear();
    m_taskLabels.clear();
    m_partial.clear();
    saveSettings();
    event->accept();
}
