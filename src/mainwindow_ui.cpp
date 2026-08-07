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
#include <QAction>
#include <QKeyEvent>
#include <QContextMenuEvent>
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
#include <QAbstractItemView>
#include <QTabWidget>
#include <QStackedWidget>
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
#include <QSplitter>
#include <QOverload>

// ------------------------------------------------------------------
// 顶部各输入行的统一 label 宽度：保证程序/ffmpeg/链接/保存文件名
// 各行的输入框在纵向对齐。
// ------------------------------------------------------------------
static constexpr int kFieldLabelWidth = 80;

// ------------------------------------------------------------------
// 粘贴时自动去除首尾空白的输入框（用于保存文件名等）。
// ------------------------------------------------------------------
class TrimmedPasteLineEdit : public QLineEdit {
public:
    using QLineEdit::QLineEdit;
protected:
    void keyPressEvent(QKeyEvent *e) override
    {
        // Ctrl+V / Shift+Insert 粘贴时去除首尾空白。
        if ((e->key() == Qt::Key_V && e->modifiers().testFlag(Qt::ControlModifier))
                || (e->key() == Qt::Key_Insert && e->modifiers().testFlag(Qt::ShiftModifier))) {
            insert(QGuiApplication::clipboard()->text().trimmed());
            return;
        }
        QLineEdit::keyPressEvent(e);
    }
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        // 完全自定义右键菜单，避免依赖标准菜单里难以识别的"Paste"项。
        QMenu menu(this);

        QAction *undoAct = menu.addAction("撤销", this, &QLineEdit::undo);
        undoAct->setShortcut(QKeySequence::Undo);
        undoAct->setEnabled(isUndoAvailable());
        QAction *redoAct = menu.addAction("重做", this, &QLineEdit::redo);
        redoAct->setShortcut(QKeySequence::Redo);
        redoAct->setEnabled(isRedoAvailable());

        menu.addSeparator();

        QAction *cutAct = menu.addAction("剪切", this, &QLineEdit::cut);
        cutAct->setShortcut(QKeySequence::Cut);
        cutAct->setEnabled(!isReadOnly() && hasSelectedText());
        QAction *copyAct = menu.addAction("复制", this, &QLineEdit::copy);
        copyAct->setShortcut(QKeySequence::Copy);
        copyAct->setEnabled(hasSelectedText());
        QAction *pasteAct = menu.addAction("粘贴", this, [this]() {
            insert(QGuiApplication::clipboard()->text().trimmed());  // 去首尾空格
        });
        pasteAct->setShortcut(QKeySequence::Paste);
        pasteAct->setEnabled(!isReadOnly());
        QAction *delAct = menu.addAction("删除", this, &QLineEdit::del);
        delAct->setShortcut(QKeySequence::Delete);
        delAct->setEnabled(!isReadOnly() && hasSelectedText());

        menu.addSeparator();

        QAction *selAllAct = menu.addAction("全选", this, &QLineEdit::selectAll);
        selAllAct->setShortcut(QKeySequence::SelectAll);
        selAllAct->setEnabled(!text().isEmpty());

        menu.exec(event->globalPos());
    }
};

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

void MainWindow::buildPathRow()
{
    // 程序 / ffmpeg 路径各占一行，置于链接输入上方，label 统一宽度使输入框纵向对齐。
    auto *exeRow = new QHBoxLayout();
    auto *exeLabel = new QLabel("程序:");
    exeLabel->setFixedWidth(kFieldLabelWidth);
    exeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    exeRow->addWidget(exeLabel);
    m_exePath = new QLineEdit();
    m_exePath->setPlaceholderText("N_m3u8DL-RE 可执行文件 (如 N_m3u8DL-RE.exe)");
    exeRow->addWidget(m_exePath, 1);
    m_exeBtn = new QPushButton("浏览...");
    connect(m_exeBtn, &QPushButton::clicked,
            this, &MainWindow::browseExe);
    exeRow->addWidget(m_exeBtn);
    m_root->addLayout(exeRow);

    auto *ffRow = new QHBoxLayout();
    auto *ffLabel = new QLabel("ffmpeg:");
    ffLabel->setFixedWidth(kFieldLabelWidth);
    ffLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ffRow->addWidget(ffLabel);
    m_ffmpegPath = new QLineEdit();
    m_ffmpegPath->setPlaceholderText("ffmpeg 可执行文件 (可选, 留空则自动查找)");
    ffRow->addWidget(m_ffmpegPath, 1);
    m_ffmpegBtn = new QPushButton("浏览...");
    connect(m_ffmpegBtn, &QPushButton::clicked,
            this, [this]() { browseFor(m_ffmpegPath, false); });
    ffRow->addWidget(m_ffmpegBtn);
    m_root->addLayout(ffRow);

    connect(m_exePath, &QLineEdit::textChanged,
            this, [this]() { onPathTextChanged(m_exePath); });
    connect(m_ffmpegPath, &QLineEdit::textChanged,
            this, [this]() { onPathTextChanged(m_ffmpegPath); });
}

void MainWindow::buildInputRow()
{
    // 链接输入 + 开始下载按钮：置顶、醒目。
    auto *row = new QHBoxLayout();
    auto *linkLabel = new QLabel("链接:");
    linkLabel->setFixedWidth(kFieldLabelWidth);
    linkLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(linkLabel);
    m_input = new QLineEdit();
    m_input->installEventFilter(this);  // 双击链接框时自动派生保存文件名
    m_input->setPlaceholderText("输入 m3u8/dash 链接或本地文件 (input)");
    m_input->setMinimumHeight(28);
    row->addWidget(m_input, 1);
    m_startBtn = new QPushButton("开始下载");
    m_startBtn->setMinimumHeight(28);
    connect(m_startBtn, &QPushButton::clicked,
            this, &MainWindow::startDownload);
    row->addWidget(m_startBtn);
    m_root->addLayout(row);

    connect(m_input, &QLineEdit::returnPressed,
            this, &MainWindow::startDownload);
}

void MainWindow::buildControlRow()
{
    // 保存文件名：占满整行，输入框与上方程序/ffmpeg/链接行同宽对齐。
    auto *row = new QHBoxLayout();
    auto *saveLabel = new QLabel("保存文件名:");
    saveLabel->setFixedWidth(kFieldLabelWidth);
    saveLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(saveLabel);
    m_saveNameEdit = new TrimmedPasteLineEdit();
    m_saveNameEdit->installEventFilter(this);
    connect(m_saveNameEdit, &QLineEdit::returnPressed,
            this, &MainWindow::startDownload);
    row->addWidget(m_saveNameEdit, 1);
    m_root->addLayout(row);

    // 按钮行：停止 / 打开 + 任务数量。
    auto *btnRow = new QHBoxLayout();
    m_stopBtn = new QPushButton("停止所有");
    m_stopBtn->setEnabled(false);
    connect(m_stopBtn, &QPushButton::clicked,
            this, &MainWindow::stopDownload);
    btnRow->addWidget(m_stopBtn);
    m_openBtn = new QPushButton("打开输出目录");
    connect(m_openBtn, &QPushButton::clicked,
            this, &MainWindow::openOutputDir);
    btnRow->addWidget(m_openBtn);
    btnRow->addStretch(1);
    m_taskCountLabel = new QLabel("运行中: 0  等待中: 0");
    btnRow->addWidget(m_taskCountLabel);
    m_root->addLayout(btnRow);
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

QWidget *MainWindow::buildSettingsPage()
{
    auto *page = new QWidget();
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(6);

    // ====== 运行控制：并发限制 + 独立终端（低频一次性设置，与 CLI 选项同放设置页）======
    {
        auto *runHeader = new QLabel("运行控制");
        runHeader->setStyleSheet("font-weight: bold; color: #0078d4; padding-top: 4px;");
        pageLayout->addWidget(runHeader);

        auto *runBar = new QHBoxLayout();
        auto *concLabel = new QLabel("并发:");
        concLabel->setFixedWidth(kFieldLabelWidth);
        concLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        runBar->addWidget(concLabel);
        m_queueEnabledBox = new QCheckBox("限制");
        runBar->addWidget(m_queueEnabledBox);
        m_maxConcurrentBox = new QSpinBox();
        m_maxConcurrentBox->setRange(1, 99);
        m_maxConcurrentBox->setValue(5);
        m_maxConcurrentBox->setEnabled(false);
        runBar->addWidget(m_maxConcurrentBox);
        runBar->addSpacing(16);
        m_terminalModeBox = new QCheckBox("独立终端");
        m_terminalModeBox->setToolTip("下载时在新终端窗口中运行，不经过队列");
        runBar->addWidget(m_terminalModeBox);
        runBar->addStretch(1);
        pageLayout->addLayout(runBar);

        connect(m_queueEnabledBox, &QCheckBox::toggled,
                this, &MainWindow::onQueueToggled);
        connect(m_maxConcurrentBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MainWindow::onMaxConcurrentChanged);
    }

    // ====== 参数设置：左侧分类导航 + 右侧内容区 ======
    auto *settingsSplit = new QHBoxLayout();
    settingsSplit->setSpacing(8);

    m_settingsNav = new QListWidget();
    m_settingsNav->setObjectName("settingsNav");
    m_settingsNav->setSelectionMode(QAbstractItemView::SingleSelection);
    m_settingsNav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_settingsNav->setFixedWidth(110);
    settingsSplit->addWidget(m_settingsNav);

    m_settingsStack = new QStackedWidget();
    settingsSplit->addWidget(m_settingsStack, 1);

    const QVector<Category> cats = buildCategories();
    for (const Category &cat : cats) {
        auto *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);

        auto *page_ = new QWidget(scroll);
        auto *vbox = new QVBoxLayout(page_);
        vbox->setSpacing(6);

        // 统一创建选项控件；--save-name 复用主界面输入框，不放入网格。
        auto makeEntry = [&](const Opt &o, QWidget **field, QWidget **valueWidget) -> bool {
            if (o.flag == "--save-name") {
                Entry e{o, m_saveNameEdit};
                m_entries.append(e);
                connectEntryPreview(e);
                return false;
            }
            *field = makeOptionWidget(o, valueWidget);
            Entry e{o, *valueWidget};
            m_entries.append(e);
            connectEntryPreview(e);
            return true;
        };

        auto addGroupHeader = [&](const QString &title) {
            auto *header = new QLabel(title, page_);
            header->setStyleSheet("font-weight: bold; color: #0078d4; padding-top: 4px;");
            vbox->addWidget(header);
        };

        for (const OptGroup &g : cat.groups) {
            if (g.opts.isEmpty())
                continue;
            addGroupHeader(g.title);

            QVector<Opt> bools, others;
            for (const Opt &o : g.opts)
                (o.type == Opt::Bool ? bools : others).append(o);

            // 开关项：复选框网格
            if (!bools.isEmpty()) {
                auto *grid = new QGridLayout();
                grid->setHorizontalSpacing(12);
                grid->setVerticalSpacing(4);
                int row = 0, col = 0;
                for (const Opt &o : bools) {
                    QWidget *valueWidget = nullptr;
                    QWidget *field = nullptr;
                    if (!makeEntry(o, &field, &valueWidget))
                        continue;
                    if (auto *cb = qobject_cast<QCheckBox *>(field))
                        cb->setText(o.label);
                    grid->addWidget(field, row, col);
                    if (++col >= 3) { col = 0; ++row; }
                }
                vbox->addLayout(grid);
            }

            // 其他项：label 在上、控件在下的两列网格
            if (!others.isEmpty()) {
                auto *grid = new QGridLayout();
                grid->setHorizontalSpacing(16);
                grid->setVerticalSpacing(8);
                int row = 0, col = 0;
                for (const Opt &o : others) {
                    QWidget *valueWidget = nullptr;
                    QWidget *field = nullptr;
                    if (!makeEntry(o, &field, &valueWidget))
                        continue;
                    auto *cell = new QWidget();
                    auto *cellLay = new QVBoxLayout(cell);
                    cellLay->setContentsMargins(0, 0, 0, 0);
                    auto *label = new QLabel(o.label, page_);
                    label->setToolTip(o.tooltip);
                    cellLay->addWidget(label);
                    cellLay->addWidget(field);
                    grid->addWidget(cell, row, col);
                    if (++col >= 2) { col = 0; ++row; }
                }
                vbox->addLayout(grid);
            }
        }

        vbox->addStretch(1);
        scroll->setWidget(page_);
        m_settingsStack->addWidget(scroll);
        m_settingsNav->addItem(cat.name);
    }

    connect(m_settingsNav, &QListWidget::currentRowChanged,
            m_settingsStack, &QStackedWidget::setCurrentIndex);
    if (m_settingsNav->count() > 0)
        m_settingsNav->setCurrentRow(0);

    m_settingsNav->setStyleSheet(R"(
        QListWidget#settingsNav {
            background: transparent;
            border: none;
            outline: none;
            padding: 4px 2px;
        }
        QListWidget#settingsNav::item {
            padding: 8px 10px;
            border-radius: 4px;
            margin: 1px 2px;
            font-size: 13px;
        }
        QListWidget#settingsNav::item:hover:!selected {
            background-color: rgba(128, 128, 128, 50);
        }
        QListWidget#settingsNav::item:selected {
            background-color: palette(highlight);
            color: palette(highlighted-text);
        }
    )");
    pageLayout->addLayout(settingsSplit, 1);

    // ====== 命令预览 ======
    {
        auto *cmdRow = new QHBoxLayout();
        cmdRow->addWidget(new QLabel("命令预览:"));
        cmdRow->addStretch(1);
        m_copyBtn = new QPushButton("复制");
        connect(m_copyBtn, &QPushButton::clicked,
                this, &MainWindow::copyCommand);
        cmdRow->addWidget(m_copyBtn);
        pageLayout->addLayout(cmdRow);

        m_cmdPreview = new QPlainTextEdit();
        m_cmdPreview->setReadOnly(true);
        m_cmdPreview->setLineWrapMode(QPlainTextEdit::NoWrap);
        m_cmdPreview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        m_cmdPreview->setMinimumHeight(48);
        m_cmdPreview->setMaximumHeight(96);
        pageLayout->addWidget(m_cmdPreview);
    }

    return page;
}

QWidget *MainWindow::buildDownloadPage()
{
    auto *page = new QWidget();
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal);

    // 左侧：下载列表
    auto *taskPanel = new QWidget();
    auto *taskLayout = new QVBoxLayout(taskPanel);
    taskLayout->setContentsMargins(0, 0, 0, 0);
    taskLayout->addWidget(new QLabel("下载列表:"));
    m_taskList = new QListWidget();
    m_taskList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_taskList->addItem("全部");
    taskLayout->addWidget(m_taskList);
    connect(m_taskList, &QListWidget::itemClicked,
            this, &MainWindow::onTaskListClicked);
    connect(m_taskList, &QWidget::customContextMenuRequested,
            this, &MainWindow::onTaskListContextMenu);

    m_taskFilter = new QComboBox();
    m_taskFilter->addItem("全部");
    m_taskFilter->setVisible(false);
    connect(m_taskFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTaskFilterChanged);
    taskLayout->addWidget(m_taskFilter);

    splitter->addWidget(taskPanel);

    // 右侧：日志
    m_log = new QPlainTextEdit();
    m_log->setReadOnly(true);
    m_log->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_log->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_log, &QWidget::customContextMenuRequested,
            this, &MainWindow::onLogContextMenu);
    splitter->addWidget(m_log);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 500});
    pageLayout->addWidget(splitter, 1);

    return page;
}

void MainWindow::buildCenterTabs()
{
    m_centerTabs = new QTabWidget();
    m_centerTabs->setObjectName("centerTabs");
    m_centerTabs->addTab(buildDownloadPage(), "下载列表");
    m_centerTabs->addTab(buildSettingsPage(), "参数设置");

    m_centerTabs->setStyleSheet(R"(
        QTabWidget#centerTabs::pane { border: none; }
        QTabBar::tab {
            padding: 8px 22px;
            font-size: 13px;
            border: none;
            border-bottom: 2px solid transparent;
            margin-right: 4px;
        }
        QTabBar::tab:selected {
            font-weight: bold;
            border-bottom: 3px solid #0078d4;
        }
        QTabBar::tab:hover:!selected {
            border-bottom: 3px solid #90c0e0;
        }
    )");

    m_root->addWidget(m_centerTabs, 1);
}

void MainWindow::setupTabOrder()
{
    // 严格按界面从上到下、每行从左到右排列。
    QVector<QWidget *> chain;
    chain << m_exePath << m_exeBtn                       // 程序 / ffmpeg 行
          << m_ffmpegPath << m_ffmpegBtn;
    chain << m_input << m_startBtn;                      // 链接行
    chain << m_saveNameEdit;                             // 保存文件名行
    chain << m_stopBtn << m_openBtn;                     // 按钮行
    chain << m_centerTabs;                               // 下载列表 | 参数设置
    chain << m_queueEnabledBox << m_maxConcurrentBox     // 下载页控制栏
          << m_terminalModeBox;
    for (const Entry &e : m_entries) {
        // --save-name 复用 m_saveNameEdit，它已在上方主链中，
        // 不能在这里重复链接，否则会覆盖主链顺序。
        if (e.widget == m_saveNameEdit)
            continue;
        chain.append(e.widget);
    }
    chain << m_cmdPreview << m_copyBtn;                  // 命令预览
    chain << m_taskList << m_taskFilter << m_log;        // 下载列表 | 日志

    for (int i = 0; i + 1 < chain.size(); ++i)
        setTabOrder(chain.at(i), chain.at(i + 1));
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
