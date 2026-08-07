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
    m_cmdPreview->setPlainText(previewCommand(program, args));
}

void MainWindow::copyCommand()
{
    QGuiApplication::clipboard()->setText(m_cmdPreview->toPlainText());
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
    // 与 buildArguments 的默认行为一致：未填写时使用 <程序目录>/download。
    if (dir.isEmpty())
        dir = QDir(QApplication::applicationDirPath()).filePath("download");
    const QString abs = QDir::isAbsolutePath(dir)
                            ? QDir::cleanPath(dir)
                            : QDir(QApplication::applicationDirPath()).filePath(dir);
    QDir().mkpath(abs);  // 确保目录存在

#ifdef Q_OS_WIN
    // 直接调用资源管理器打开目录，避免 QDesktopServices 在部分系统上
    // 把目录 open 关联到终端等默认处理程序。
    QProcess::startDetached("explorer.exe", {QDir::toNativeSeparators(abs)});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(abs));
#endif
}
