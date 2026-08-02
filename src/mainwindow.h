#pragma once

#include <QMainWindow>
#include <QVector>
#include <QHash>

#include "options.h"

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QTabWidget;
class QVBoxLayout;
class QCloseEvent;
class QAction;
class QActionGroup;
class StringListWidget;

#include <QProcess>

// One generated option widget paired with its metadata.
struct Entry {
    Opt opt;
    QWidget *widget = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void browseExe();
    void browseFor(QLineEdit *target, bool dirOnly);

    void generatePreview();
    void copyCommand();
    void startDownload();
    void stopDownload();

    void readProcessOutput();
    void processFinished(int exitCode, QProcess::ExitStatus status);

    void openOutputDir();
    void about();
    void requestExit();
    void resetSettings();
    void resetWidgetsToDefaults();

    void onThemeActionTriggered(QAction *action);
    void setTheme(const QString &name);
    void onAlwaysOnTopToggled(bool on);

    void updateTaskCount();

private:
    void buildToolbar();
    void buildInputArea();
    void buildOptionsUi();
    void buildRunArea();

    QWidget *makeOptionWidget(const Opt &opt, QWidget **valueWidget);
    void connectEntryPreview(const Entry &e);
    void appendLog(const QString &text);
    void applyUiSettings();

    QStringList buildArguments();
    QString previewCommand(const QString &program, const QStringList &args);

    void saveSettings();
    void loadSettings();
    void autoDetectExe();
    void autoDetectFfmpeg();

    QString findInStandardLocations(const QString &exeName) const;
    void refreshPathHint(QLineEdit *edit, const QString &exeName);
    void onPathTextChanged(QLineEdit *edit);
    QString relativeDisplay(const QString &absPath) const;
    QString effectiveAbsolute(QLineEdit *edit) const;
    QString resolvedExePath() const;
    QString resolvedFfmpegPath() const;

    QLineEdit *m_exePath = nullptr;
    QLineEdit *m_ffmpegPath = nullptr;
    QLineEdit *m_input = nullptr;
    QTabWidget *m_tabs = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QLineEdit *m_cmdPreview = nullptr;
    QPushButton *m_startBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QLabel *m_taskCountLabel = nullptr;

    QAction *m_alwaysOnTopAction = nullptr;
    QVector<QAction *> m_themeActions;
    QString m_currentTheme = "跟随系统";

    QVBoxLayout *m_root = nullptr;
    QVector<QProcess *> m_tasks;
    QHash<QProcess *, QString> m_taskLabels;  // 进程 -> 日志标签 (如 "[#1 url]")
    QHash<QProcess *, QString> m_partial;     // 进程 -> 未换行的不完整行缓冲
    int m_nextTaskId = 0;
    QVector<Entry> m_entries;
};
