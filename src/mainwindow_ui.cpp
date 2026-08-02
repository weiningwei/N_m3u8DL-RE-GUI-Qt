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
    m_exeBtn = exeBtn;
    connect(exeBtn, &QPushButton::clicked, this, &MainWindow::browseExe);
    exeRow->addWidget(exeBtn);
    m_root->addLayout(exeRow);

    auto *ffRow = new QHBoxLayout();
    ffRow->addWidget(new QLabel("ffmpeg路径:"));
    m_ffmpegPath = new QLineEdit();
    m_ffmpegPath->setPlaceholderText("ffmpeg 可执行文件 (可选, 留空则自动查找)");
    ffRow->addWidget(m_ffmpegPath, 1);
    auto *ffBtn = new QPushButton("浏览...");
    m_ffmpegBtn = ffBtn;
    connect(ffBtn, &QPushButton::clicked,
            this, [this]() { browseFor(m_ffmpegPath, false); });
    ffRow->addWidget(ffBtn);
    m_root->addLayout(ffRow);

    connect(m_exePath, &QLineEdit::textChanged,
            this, [this]() { onPathTextChanged(m_exePath); });
    connect(m_ffmpegPath, &QLineEdit::textChanged,
            this, [this]() { onPathTextChanged(m_ffmpegPath); });
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
        // 每个分类放进滚动区域：选项少时不留白，选项多时滚动而非撑大窗口。
        auto *scroll = new QScrollArea(m_tabs);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);

        auto *page = new QWidget(scroll);
        auto *vbox = new QVBoxLayout(page);
        vbox->setSpacing(8);

        // 单列表单用：标签在左、控件在右。
        auto addFormRow = [&](const Opt &o, QFormLayout *form) {
            QWidget *valueWidget = nullptr;
            QWidget *field = makeOptionWidget(o, &valueWidget);
            auto *label = new QLabel(o.label, page);
            label->setToolTip(o.tooltip);
            form->addRow(label, field);
            Entry e{o, valueWidget};
            m_entries.append(e);
            connectEntryPreview(e);
            if (o.flag == "--save-name") {
                m_saveNameEdit = qobject_cast<QLineEdit *>(valueWidget);
                m_saveNameEdit->installEventFilter(this);
                connect(m_saveNameEdit, &QLineEdit::returnPressed,
                        this, &MainWindow::startDownload);
            }
        };

        if (cat.twoColumn) {
            // 开关类参数集中为 3 列紧凑复选框（复选框自带文字）。
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
                        cb->setText(o.label); // 复选框自带文字，更紧凑
                    grid->addWidget(field, row, col);
                    Entry e{o, valueWidget};
                    m_entries.append(e);
                    connectEntryPreview(e);
                    if (++col >= 3) {   // 每行三个开关
                        col = 0;
                        ++row;
                    }
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
                    if (++col >= 2) {   // 其余参数每行两个，相邻为功能相近项
                        col = 0;
                        ++row;
                    }
                }
                vbox->addLayout(grid);
            }
        } else {
            auto *form = new QFormLayout();
            form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            form->setLabelAlignment(Qt::AlignLeft);
            form->setHorizontalSpacing(10);
            bool inputInjected = false;
            for (const Opt &o : cat.opts) {
                // 将"链接/文件"输入行置于"保存文件名"之上。
                if (!inputInjected && o.flag == "--save-name") {
                    if (!m_input) {
                        m_input = new QLineEdit(page);
                        m_input->setPlaceholderText("输入 m3u8/dash 链接或本地文件 (input)");
                        connect(m_input, &QLineEdit::returnPressed,
                                this, &MainWindow::startDownload);
                    }
                    form->addRow(new QLabel("链接/文件:", page), m_input);
                    inputInjected = true;
                }
                addFormRow(o, form);
            }
            vbox->addLayout(form);
        }

        vbox->addStretch(1); // 内容靠上
        scroll->setWidget(page);
        m_tabs->addTab(scroll, cat.name);
    }
    // 面板按内容高度自适应（去除拉伸），并限制最大高度，多余的垂直空间都给日志区。
    m_tabs->setMaximumHeight(360);
    m_root->addWidget(m_tabs);
}

void MainWindow::buildRunArea()
{
    auto *prevRow = new QHBoxLayout();
    prevRow->addWidget(new QLabel("命令预览:"));
    m_cmdPreview = new QLineEdit();
    m_cmdPreview->setReadOnly(true);
    prevRow->addWidget(m_cmdPreview, 1);
    auto *copyBtn = new QPushButton("复制");
    m_copyBtn = copyBtn;
    connect(copyBtn, &QPushButton::clicked, this, &MainWindow::copyCommand);
    prevRow->addWidget(copyBtn);
    m_root->addLayout(prevRow);

    auto *btnRow = new QHBoxLayout();
    m_startBtn = new QPushButton("开始下载");
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setEnabled(false);
    auto *genBtn = new QPushButton("生成命令");
    m_genBtn = genBtn;
    auto *openBtn = new QPushButton("打开输出目录");
    m_openBtn = openBtn;
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

void MainWindow::setupTabOrder()
{
    // 按视觉布局设置 Tab 键焦点顺序：顶部路径 -> 选项卡 -> 各选项(链接框在保存文件名之前) -> 命令预览/按钮 -> 日志过滤。
    QVector<QWidget *> chain;
    chain << m_exePath << m_exeBtn << m_ffmpegPath << m_ffmpegBtn << m_tabs;

    int saveNameIdx = -1;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).opt.flag == "--save-name") {
            saveNameIdx = i;
            break;
        }
    }
    for (int i = 0; i < m_entries.size(); ++i) {
        if (i == saveNameIdx && m_input)   // 链接/文件 置于 保存文件名 之前
            chain.append(m_input);
        chain.append(m_entries.at(i).widget);
    }
    if (saveNameIdx < 0 && m_input)
        chain.append(m_input);   // 兜底

    chain << m_cmdPreview << m_copyBtn << m_startBtn << m_stopBtn
          << m_genBtn << m_openBtn << m_taskFilter;

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
