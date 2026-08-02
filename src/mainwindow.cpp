#include "mainwindow.h"

#include <QVBoxLayout>
#include <QGuiApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QEvent>
#include <QLineEdit>
#include <QProcess>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("N_m3u8DL-RE GUI (Qt)");
    resize(920, 700);

    auto *central = new QWidget(this);
    m_root = new QVBoxLayout(central);
    m_root->setSpacing(6);
    setCentralWidget(central);

    buildToolbar();
    buildInputRow();         // 链接 + 开始下载
    buildControlRow();       // 保存文件名 + 并发 + 按钮 + 计数
    buildFoldableSettings(); // ▶ 设置（可折叠面板）
    buildMonitorArea();      // 下载列表 | 日志 (QSplitter)

    loadSettings();
    autoDetectExe();
    autoDetectFfmpeg();
    generatePreview();

    connect(m_input, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
    connect(m_input, &QLineEdit::textChanged, this, &MainWindow::onInputChanged);
    connect(m_exePath, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::onClipboardDataChanged);

    // 启动时若链接框为空且剪贴板是链接/地址，则自动填入。
    autoFillFromClipboard();

    setupTabOrder();

    m_input->setFocus();
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

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 双击"保存文件名"时，从链接重新派生文件名。
    if (watched == m_saveNameEdit && event->type() == QEvent::MouseButtonDblClick) {
        const QString name = deriveName(m_input->text().trimmed());
        if (!name.isEmpty()) {
            m_saveNameEdit->setText(name);
            m_lastAutoName = name;
        }
        return true; // 已处理，不再执行默认的双击选中词
    }
    return QMainWindow::eventFilter(watched, event);
}

// 便携模式：所有设置保存在程序目录下的 config.ini
QSettings portableSettings()
{
    return QSettings(QGuiApplication::applicationDirPath() + "/config.ini",
                     QSettings::IniFormat);
}
