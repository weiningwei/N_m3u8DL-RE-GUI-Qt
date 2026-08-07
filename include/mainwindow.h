#pragma once

#include <QMainWindow>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QSettings>

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
class QCheckBox;
class QSpinBox;
class QListWidget;
class QListWidgetItem;
class QToolButton;
class QStackedWidget;

#include <QProcess>

// One generated option widget paired with its metadata.
struct Entry {
    Opt opt;
    QWidget *widget = nullptr;
};

// A single log line together with the task tag it belongs to.
struct LogLine {
    QString tag;    // 如 "[#1 url]"；空串表示与任务无关的界面日志
    QString level;  // 日志级别：INFO / WARNING / ERROR
    QString time;   // 操作时间：yyyy-MM-dd HH:mm:ss
    QString text;
};

// Metadata for a download task, kept after completion so its log stays filterable.
struct TaskMeta {
    int id = 0;
    QString tag;        // 与 LogLine::tag 对应
    QString label;      // 下拉中显示的简短描述
    QString input;      // 原始链接/文件，用于右键复制
    bool active = true;
    bool finished = false;  // 是否已结束
    bool success = false;   // 是否正常结束
    int exitCode = 0;
    bool waiting = false;   // 是否已加入等待队列但尚未启动
    bool stopping = false;  // 是否正在被停止
};

// A download request captured at submission time, so queued tasks keep the
// exact command line the user entered even if the input box changes later.
struct PendingTask {
    QString program;
    QStringList args;
    int id = 0;
    QString tag;
    QString label;
    TaskMeta meta;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void browseExe();
    void browseFor(QLineEdit *target, bool dirOnly);

    void generatePreview();
    void copyCommand();
    void startDownload();
    void stopDownload();

    void startTaskNow(const PendingTask &pt);
    void enqueueTask(const PendingTask &pt);
    void maybeStartQueued();
    void onQueueToggled(bool on);
    void onMaxConcurrentChanged(int v);
    void onPresetSelected(int index);
    void onSavePreset();
    void onDeletePreset();
    void removeTaskByTag(const QString &tag);
    void setTaskListText(const QString &tag, const QString &text);
    void stopTaskByTag(const QString &tag);
    void cancelWaitingByTag(const QString &tag);
    void clearLog();
    void copyAllLog();
    void copyVisibleLog();

    void readProcessOutput();
    void processFinished(int exitCode, QProcess::ExitStatus status);

    void openOutputDir();
    void about();
    void requestExit();
    bool confirmExit();
    void resetSettings();
    void resetWidgetsToDefaults();

    void onThemeActionTriggered(QAction *action);
    void setTheme(const QString &name);
    void onAlwaysOnTopToggled(bool on);

    void updateTaskCount();

    void onTaskFilterChanged(int index);
    void onTaskListClicked(QListWidgetItem *item);
    void onTaskListContextMenu(const QPoint &pos);
    void onLogContextMenu(const QPoint &pos);
    QString currentFilterTag() const;
    void refreshLogView();

private:
    void buildToolbar();
    void buildInputRow();
    void buildControlRow();
    void buildFoldableSettings();
    void buildMonitorArea();
    void setupTabOrder();

    QWidget *makeOptionWidget(const Opt &opt, QWidget **valueWidget);
    void connectEntryPreview(const Entry &e);
    void appendLog(const QString &text, const QString &tag = QString(),
                   const QString &level = QStringLiteral("INFO"));
    void applyUiSettings();

    QStringList buildArguments();
    QString previewCommand(const QString &program, const QStringList &args);
    QString taskDisplay(const TaskMeta &m) const;

    void saveSettings();
    void loadSettings();
    void savePreset(const QString &name);
    void loadPreset(const QString &name);
    void refreshPresetList();
    static QString presetDirPath();
    void autoDetectExe();
    void autoDetectFfmpeg();

    QString findInStandardLocations(const QString &exeName) const;
    void refreshPathHint(QLineEdit *edit, const QString &exeName);
    void markDetected(QLineEdit *edit);
    void onPathTextChanged(QLineEdit *edit);
    QString relativeDisplay(const QString &absPath) const;
    QString effectiveAbsolute(QLineEdit *edit) const;
    QString resolvedExePath() const;
    QString resolvedFfmpegPath() const;

    void onInputChanged();
    void maybeAutoFillFromClipboard();
    static QString deriveName(const QString &input);
    static bool looksLikeUrl(const QString &text);

    QLineEdit *m_exePath = nullptr;
    QLineEdit *m_ffmpegPath = nullptr;
    QLineEdit *m_input = nullptr;
    QLineEdit *m_saveNameEdit = nullptr;  // 保存文件名输入框（--save-name）
    QString m_lastAutoName;               // 最近一次自动派生的文件名，用于避免覆盖手动输入
    QString m_lastClipUrl;                // 最近一次自动从剪贴板捕获的链接，用于实时监听时避免覆盖手动输入
    QListWidget *m_settingsNav = nullptr;      // 参数设置侧边栏
    QStackedWidget *m_settingsStack = nullptr; // 参数设置内容区
    QPlainTextEdit *m_log = nullptr;
    QLineEdit *m_cmdPreview = nullptr;
    QPushButton *m_startBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QLabel *m_taskCountLabel = nullptr;
    QComboBox *m_taskFilter = nullptr;
    QListWidget *m_taskList = nullptr;   // 下载列表（可视化 + 点击筛选日志）
    QPushButton *m_exeBtn = nullptr;
    QPushButton *m_ffmpegBtn = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QPushButton *m_genBtn = nullptr;
    QPushButton *m_openBtn = nullptr;
    QToolButton *m_settingsToggle = nullptr;   // 「参数设置」展开/折叠按钮
    QWidget *m_settingsPanel = nullptr;        // 折叠式参数面板容器

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
    QVector<PendingTask> m_waiting; // 等待队列（受并发数量限制时暂存）
    QSet<QString> m_downloadingInputs; // 正在下载的链接
    QSet<QString> m_queuedInputs;      // 排队中的链接
    QSet<QString> m_completedInputs;   // 已下载完成的链接
    QCheckBox *m_queueEnabledBox = nullptr;  // 「限制同时下载数量」
    QSpinBox *m_maxConcurrentBox = nullptr;  // 「同时最多下载 N 个」
    QCheckBox *m_terminalModeBox = nullptr;  // 「独立终端」
    QComboBox *m_presetCombo = nullptr;      // 预设选择
    QLineEdit *m_presetNameEdit = nullptr;   // 预设名称输入
    QPushButton *m_presetSaveBtn = nullptr;  // 保存预设
    QPushButton *m_presetDelBtn = nullptr;   // 删除预设
    QString m_currentPreset;                  // 当前活动预设名
};

// 便携模式：所有设置保存在程序目录下的 config.ini
QSettings portableSettings();
