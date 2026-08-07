#include "mainwindow.h"

#include <QVBoxLayout>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QLineEdit>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>

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
    buildPathRow();        // 程序 / ffmpeg 路径（合并一行，置于链接上方）
    buildInputRow();       // 链接 + 开始下载
    buildControlRow();     // 保存文件名 + 并发 + 按钮 + 计数
    buildCenterTabs();     // 下载列表 | 参数设置（两个独立页面）

    loadSettings();
    autoDetectExe();
    autoDetectFfmpeg();
    generatePreview();

    connect(m_input, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
    connect(m_input, &QLineEdit::textChanged, this, &MainWindow::onInputChanged);
    connect(m_exePath, &QLineEdit::textChanged, this, &MainWindow::generatePreview);
    // 仅当应用切换到前台（获得焦点）时才自动获取剪贴板链接，
    // 不再在后台运行时持续监听剪贴板变化。
    connect(static_cast<QGuiApplication *>(QGuiApplication::instance()),
            &QGuiApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive)
                    maybeAutoFillFromClipboard();
            });

    // 启动时若链接框为空且剪贴板是链接/地址，则自动填入。
    maybeAutoFillFromClipboard();

    setupTabOrder();

    m_input->setFocus();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmExit()) {
        event->ignore();
        return;
    }

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

bool MainWindow::confirmExit()
{
    if (!m_tasks.isEmpty() || !m_waiting.isEmpty()) {
        const int total = m_tasks.size() + m_waiting.size();
        const QMessageBox::StandardButton r = QMessageBox::question(
            this, "确认退出",
            QString("当前有 %1 个下载任务（运行中 %2 / 等待中 %3），确定要退出吗？")
                .arg(total).arg(m_tasks.size()).arg(m_waiting.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        return r == QMessageBox::Yes;
    }
    return true;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 双击"链接"框：从链接重新派生文件名。
    if (watched == m_input && event->type() == QEvent::MouseButtonDblClick) {
        const QString name = deriveName(m_input->text().trimmed());
        if (!name.isEmpty()) {
            m_saveNameEdit->setText(name);
            m_lastAutoName = name;
        }
        return true; // 已处理，不再执行默认的双击选中词
    }
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
