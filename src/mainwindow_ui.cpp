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
#include <QAbstractItemView>
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
#include <QSplitter>
#include <QOverload>

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

void MainWindow::buildInputRow()
{
    // 链接输入 + 开始下载按钮：置顶、醒目。
    auto *row = new QHBoxLayout();
    row->addWidget(new QLabel("链接:"));
    m_input = new QLineEdit();
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
    // 保存文件名 + 并发限制，同一行。
    auto *row = new QHBoxLayout();
    row->addWidget(new QLabel("保存文件名:"));
    m_saveNameEdit = new QLineEdit();
    m_saveNameEdit->installEventFilter(this);
    row->addWidget(m_saveNameEdit, 1);
    row->addWidget(new QLabel("并发:"));
    m_queueEnabledBox = new QCheckBox("限制");
    row->addWidget(m_queueEnabledBox);
    m_maxConcurrentBox = new QSpinBox();
    m_maxConcurrentBox->setRange(1, 99);
    m_maxConcurrentBox->setValue(5);
    m_maxConcurrentBox->setEnabled(false);
    row->addWidget(m_maxConcurrentBox);
    m_root->addLayout(row);

    connect(m_queueEnabledBox, &QCheckBox::toggled,
            this, &MainWindow::onQueueToggled);
    connect(m_maxConcurrentBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMaxConcurrentChanged);

    // 按钮行：停止 / 生成 / 打开 + 任务数量。
    auto *btnRow = new QHBoxLayout();
    m_stopBtn = new QPushButton("停止所有");
    m_stopBtn->setEnabled(false);
    connect(m_stopBtn, &QPushButton::clicked,
            this, &MainWindow::stopDownload);
    btnRow->addWidget(m_stopBtn);
    m_genBtn = new QPushButton("生成命令");
    connect(m_genBtn, &QPushButton::clicked,
            this, &MainWindow::generatePreview);
    btnRow->addWidget(m_genBtn);
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

void MainWindow::buildFoldableSettings()
{
    // ====== 选项标签页：始终可见 ======
    m_tabs = new QTabWidget(this);
    const QVector<Category> cats = buildCategories();
    for (const Category &cat : cats) {
        auto *scroll = new QScrollArea(m_tabs);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);

        auto *page = new QWidget(scroll);
        auto *vbox = new QVBoxLayout(page);
        vbox->setSpacing(8);

        auto addFormRow = [&](const Opt &o, QFormLayout *form) {
            QWidget *valueWidget = nullptr;
            QWidget *field;
            if (o.flag == "--save-name") {
                Entry e{o, m_saveNameEdit};
                m_entries.append(e);
                connectEntryPreview(e);
                return;
            }
            field = makeOptionWidget(o, &valueWidget);
            auto *label = new QLabel(o.label, page);
            label->setToolTip(o.tooltip);
            form->addRow(label, field);
            Entry e{o, valueWidget};
            m_entries.append(e);
            connectEntryPreview(e);
        };

        if (cat.twoColumn) {
            QVector<Opt> bools, others;
            for (const Opt &o : cat.opts)
                (o.type == Opt::Bool ? bools : others).append(o);

            if (!bools.isEmpty()) {
                vbox->addWidget(new QLabel("开关选项", page));
                auto *grid = new QGridLayout();
                grid->setHorizontalSpacing(12);
                grid->setVerticalSpacing(4);
                int row = 0, col = 0;
                for (const Opt &o : bools) {
                    QWidget *valueWidget = nullptr;
                    QWidget *field = makeOptionWidget(o, &valueWidget);
                    if (auto *cb = qobject_cast<QCheckBox *>(field))
                        cb->setText(o.label);
                    grid->addWidget(field, row, col);
                    Entry e{o, valueWidget};
                    m_entries.append(e);
                    connectEntryPreview(e);
                    if (++col >= 3) { col = 0; ++row; }
                }
                vbox->addLayout(grid);
            }

            if (!others.isEmpty()) {
                vbox->addWidget(new QLabel("其他参数", page));
                auto *grid = new QGridLayout();
                grid->setHorizontalSpacing(16);
                grid->setVerticalSpacing(8);
                int row = 0, col = 0;
                for (const Opt &o : others) {
                    QWidget *valueWidget = nullptr;
                    QWidget *field = makeOptionWidget(o, &valueWidget);
                    auto *cell = new QWidget();
                    auto *cellLay = new QVBoxLayout(cell);
                    cellLay->setContentsMargins(0, 0, 0, 0);
                    auto *label = new QLabel(o.label, page);
                    label->setToolTip(o.tooltip);
                    cellLay->addWidget(label);
                    cellLay->addWidget(field);
                    grid->addWidget(cell, row, col);
                    Entry e{o, valueWidget};
                    m_entries.append(e);
                    connectEntryPreview(e);
                    if (++col >= 2) { col = 0; ++row; }
                }
                vbox->addLayout(grid);
            }
        } else {
            auto *form = new QFormLayout();
            form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            form->setLabelAlignment(Qt::AlignLeft);
            form->setHorizontalSpacing(10);
            for (const Opt &o : cat.opts)
                addFormRow(o, form);
            vbox->addLayout(form);
        }

        vbox->addStretch(1);
        scroll->setWidget(page);
        m_tabs->addTab(scroll, cat.name);
    }

    m_tabs->setStyleSheet(R"(
        QTabBar::tab {
            padding: 10px 20px;
            min-width: 60px;
            margin-right: 3px;
            font-size: 13px;
            border: none;
            border-bottom: 2px solid transparent;
        }
        QTabBar::tab:selected {
            font-weight: bold;
            border-bottom: 3px solid #0078d4;
        }
        QTabBar::tab:hover:!selected {
            border-bottom: 3px solid #90c0e0;
        }
    )");
    m_tabs->setMaximumHeight(260);
    m_root->addWidget(m_tabs);

    // ====== 可折叠"高级"面板：路径 + 命令预览 ======
    m_foldToggle = new QPushButton("▶ 高级");
    m_foldToggle->setFlat(true);
    m_foldToggle->setCursor(Qt::PointingHandCursor);
    m_foldToggle->setStyleSheet(
        "QPushButton { text-align: left; font-weight: bold; padding: 5px; "
        "border: none; font-size: 13px; }");
    connect(m_foldToggle, &QPushButton::clicked,
            this, &MainWindow::onFoldSettings);
    m_root->addWidget(m_foldToggle);

    m_foldWidget = new QWidget();
    auto *fold = new QVBoxLayout(m_foldWidget);
    fold->setContentsMargins(0, 4, 0, 8);
    fold->setSpacing(6);

    // 程序 / ffmpeg 路径
    {
        auto *exeRow = new QHBoxLayout();
        exeRow->addWidget(new QLabel("程序:"));
        m_exePath = new QLineEdit();
        m_exePath->setPlaceholderText("N_m3u8DL-RE 可执行文件 (如 N_m3u8DL-RE.exe)");
        exeRow->addWidget(m_exePath, 1);
        m_exeBtn = new QPushButton("浏览...");
        connect(m_exeBtn, &QPushButton::clicked,
                this, &MainWindow::browseExe);
        exeRow->addWidget(m_exeBtn);
        fold->addLayout(exeRow);

        auto *ffRow = new QHBoxLayout();
        ffRow->addWidget(new QLabel("ffmpeg:"));
        m_ffmpegPath = new QLineEdit();
        m_ffmpegPath->setPlaceholderText("ffmpeg 可执行文件 (可选, 留空则自动查找)");
        ffRow->addWidget(m_ffmpegPath, 1);
        m_ffmpegBtn = new QPushButton("浏览...");
        connect(m_ffmpegBtn, &QPushButton::clicked,
                this, [this]() { browseFor(m_ffmpegPath, false); });
        ffRow->addWidget(m_ffmpegBtn);
        fold->addLayout(ffRow);

        connect(m_exePath, &QLineEdit::textChanged,
                this, [this]() { onPathTextChanged(m_exePath); });
        connect(m_ffmpegPath, &QLineEdit::textChanged,
                this, [this]() { onPathTextChanged(m_ffmpegPath); });
    }

    // 命令预览
    {
        auto *cmdRow = new QHBoxLayout();
        cmdRow->addWidget(new QLabel("命令预览:"));
        m_cmdPreview = new QLineEdit();
        m_cmdPreview->setReadOnly(true);
        cmdRow->addWidget(m_cmdPreview, 1);
        m_copyBtn = new QPushButton("复制");
        connect(m_copyBtn, &QPushButton::clicked,
                this, &MainWindow::copyCommand);
        cmdRow->addWidget(m_copyBtn);
        fold->addLayout(cmdRow);
    }

    m_foldWidget->setVisible(false);
    m_root->addWidget(m_foldWidget);
}

void MainWindow::buildMonitorArea()
{
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
    m_root->addWidget(splitter, 1);
}

void MainWindow::setupTabOrder()
{
    QVector<QWidget *> chain;
    chain << m_input << m_startBtn
          << m_saveNameEdit << m_queueEnabledBox << m_maxConcurrentBox
          << m_stopBtn << m_genBtn << m_openBtn
          << m_foldToggle
          << m_exePath << m_exeBtn << m_ffmpegPath << m_ffmpegBtn;

    // 选项标签页内的控件（m_tabs 在 foldWidget 内）
    chain << m_tabs;
    for (const Entry &e : m_entries)
        chain.append(e.widget);

    chain << m_cmdPreview << m_copyBtn << m_taskFilter;

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
