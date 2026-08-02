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
class QComboBox;
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

// A single log line together with the task tag it belongs to.
struct LogLine {
    QString tag;   // 如 "[#1 url]"；空串表示与任务无关的界面日志
    QString text;
};

// Metadata for a download task, kept after completion so its log stays filterable.
struct TaskMeta {
    int id = 0;
    QString tag;        // 与 LogLine::tag 对应
    QString label;      // 下拉中显示的简短描述
    bool active = true;
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

    void onTaskFilterChanged(int index);
    QString currentFilterTag() const;
    void refreshLogView();

private:
    void buildToolbar();
    void buildInputArea();
    void buildOptionsUi();
    void buildRunArea();

    QWidget *makeOptionWidget(const Opt &opt, QWidget **valueWidget);
    void connectEntryPreview(const Entry &e);
    void appendLog(const QString &text, const QString &tag = QString());
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
    QComboBox *m_taskFilter = nullptr;

    QAction *m_alwaysOnTopAction = nullptr;
    QVector<QAction *> m_themeActions;
    QString m_currentTheme = "跟随系统";

    QVBoxLayout *m_root = nullptr;
    QVector<QProcess *> m_tasks;
    QHash<QProcess *, QString> m_taskLabels;  // 进程 -> 日志标签 (如 "[#1 url]")
    QHash<QProcess *, QString> m_partial;     // 进程 -> 未换行的不完整行缓冲
    int m_nextTaskId = 0;
    QVector<Entry> m_entries;

    QVector<LogLine> m_logLines;   // 全部日志（含任务标签），用于按任务过滤重绘
    QVector<TaskMeta> m_taskMetas; // 所有任务（含已完成），用于下拉列表
};
