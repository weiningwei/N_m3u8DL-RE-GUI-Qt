#include "mainwindow.h"

#include <QVBoxLayout>
#include <QGuiApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QEvent>
#include <QLineEdit>
#include <QProcess>

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
    connect(m_input, &QLineEdit::textChanged, this, &MainWindow::onInputChanged);
    connect(m_exePath, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::onClipboardDataChanged);

    // 启动时若链接框为空且剪贴板是链接/地址，则自动填入。
    autoFillFromClipboard();

    setupTabOrder();
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
