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
#include <QStatusBar>
#include <QOverload>

// ------------------------------------------------------------------
// 链接框自动填充与文件名派生
// ------------------------------------------------------------------

void MainWindow::onInputChanged()
{
    const QString input = m_input->text().trimmed();
    if (!m_saveNameEdit)
        return;

    if (input.isEmpty()) {
        // 链接清空时也清掉自动派生的文件名（仅当仍是自动值）。
        if (m_saveNameEdit->text() == m_lastAutoName) {
            m_saveNameEdit->clear();
            m_lastAutoName.clear();
        }
        return;
    }

    const QString name = deriveName(input);
    if (name.isEmpty())
        return;
    // 仅当用户未手动修改文件名时才自动更新，避免覆盖已有输入。
    if (m_saveNameEdit->text().isEmpty() || m_saveNameEdit->text() == m_lastAutoName) {
        m_saveNameEdit->setText(name);
        m_lastAutoName = name;
    }
}

void MainWindow::maybeAutoFillFromClipboard()
{
    const QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (!looksLikeUrl(clip))
        return;
    const QString cur = m_input->text().trimmed();
    if (clip == cur)
        return;
    // 仅当链接框为空、或当前内容仍是上次自动捕获的链接时才覆盖，避免破坏手动输入。
    if (cur.isEmpty() || cur == m_lastClipUrl) {
        m_input->setText(clip);
        m_lastClipUrl = clip;
        appendLog(QString("已从剪贴板自动获取链接: %1").arg(clip));
        statusBar()->showMessage(QString("已自动获取剪贴板链接: %1").arg(clip), 4000);
    }
}

QString MainWindow::deriveName(const QString &input)
{
    QString s = input.trimmed();
    if (s.isEmpty())
        return QString();

    // 去掉协议头
    const int scheme = s.indexOf("://");
    if (scheme >= 0)
        s = s.mid(scheme + 3);
    // 去掉查询串与片段
    int q = s.indexOf('?');
    if (q >= 0) s = s.left(q);
    int h = s.indexOf('#');
    if (h >= 0) s = s.left(h);
    // 取最后一段路径
    int slash = s.lastIndexOf('/');
    if (slash >= 0) s = s.mid(slash + 1);
    int bs = s.lastIndexOf('\\');
    if (bs >= 0) s = s.mid(bs + 1);
    // 去掉扩展名（保存文件名由工具自动补扩展名）
    const int dot = s.lastIndexOf('.');
    if (dot > 0) s = s.left(dot);
    // 清理文件名非法字符
    static const QRegularExpression bad("[<>:\"/\\\\|?*]");
    s.replace(bad, "_");
    s = s.trimmed();
    return s;
}

bool MainWindow::looksLikeUrl(const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty())
        return false;
    // 链接一般不含空白；含空白的（如命令预览）不当作链接处理。
    if (t.contains(' ') || t.contains('\t') || t.contains('\n') || t.contains('\r'))
        return false;
    if (t.contains("://"))
        return true;
    if (t.startsWith("magnet:", Qt::CaseInsensitive))
        return true;
    if (t.startsWith("rtsp:", Qt::CaseInsensitive)
            || t.startsWith("rtmp:", Qt::CaseInsensitive))
        return true;
    if (t.startsWith('/'))
        return true;
    // Windows 盘符路径，如 C:\ 或 C:/
    if (t.size() > 2 && t.at(1) == ':' && (t.at(2) == '\\' || t.at(2) == '/'))
        return true;
    return false;
}
