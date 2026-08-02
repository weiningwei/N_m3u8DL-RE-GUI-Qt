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
// Path detection & resolution
// ------------------------------------------------------------------

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
