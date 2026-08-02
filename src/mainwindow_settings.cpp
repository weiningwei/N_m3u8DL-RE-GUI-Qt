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
// Misc actions
// ------------------------------------------------------------------

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
    if (m_queueEnabledBox) {
        m_queueEnabledBox->setChecked(false);
        m_maxConcurrentBox->setValue(5);
        m_maxConcurrentBox->setEnabled(false);
    }
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
    s.setValue("queueEnabled", m_queueEnabledBox ? m_queueEnabledBox->isChecked() : false);
    s.setValue("maxConcurrent", m_maxConcurrentBox ? m_maxConcurrentBox->value() : 5);
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
    // 并发数量限制（排队模式）配置。
    const bool queueOn = s.value("queueEnabled", false).toBool();
    const int maxConc = s.value("maxConcurrent", 5).toInt();
    if (m_queueEnabledBox) {
        m_queueEnabledBox->setChecked(queueOn);
        m_maxConcurrentBox->setValue(maxConc);
        m_maxConcurrentBox->setEnabled(queueOn);
    }
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
