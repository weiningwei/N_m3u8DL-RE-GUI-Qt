#pragma once

#include <QMainWindow>
#include <QVector>

#include "options.h"

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
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

    void onThemeActionTriggered(QAction *action);
    void setTheme(const QString &name);
    void onAlwaysOnTopToggled(bool on);

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

    QAction *m_alwaysOnTopAction = nullptr;
    QVector<QAction *> m_themeActions;
    QString m_currentTheme = "跟随系统";

    QVBoxLayout *m_root = nullptr;
    QProcess *m_process = nullptr;
    QVector<Entry> m_entries;
    bool m_running = false;
};
