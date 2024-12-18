// SettingsManager.h
#pragma once

#include "qcheckbox.h"
#include <QObject>
#include <QSettings>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QRect>
#include <QStringList>
#include <memory>

class SettingsManager : public QObject {
    Q_OBJECT

public:
    // Singleton access
    static SettingsManager& getInstance();

    // Generic settings methods
    template<typename T>
    T getValue(const QString& key, const T& defaultValue = T());

    template<typename T>
    void setValue(const QString& key, const T& value);

    // Interface settings
    bool getShowTimeStamps();
    void setShowTimeStamps(bool show);

    // Directory paths
    QString getMediaDirectory();
    void setMediaDirectory(const QString& dir);
    // QString getExportDirectory();
    // void setExportDirectory(const QString& dir);
    QString getTranscriptsDirectory();
    void setTranscriptsDirectory(const QString& dir);

    // Window state management
    // void saveWindowGeometry(const QString& windowName, const QRect& geometry);
    // QRect loadWindowGeometry(const QString& windowName);
    // void saveWindowState(const QString& windowName, const QByteArray& state);
    // QByteArray loadWindowState(const QString& windowName);

    // Recent files management
    // void addRecentFile(const QString& filePath);
    // QStringList getRecentFiles();
    // void clearRecentFiles();
    // void pruneRecentFiles();
    // int getMaxRecentFiles() const;
    // void setMaxRecentFiles(int max);

    // Settings management
    // bool backup(const QString& backupPath);
    // bool restore(const QString& backupPath);
    void resetToDefaults();
    // void resetSection(const QString& section);
    bool isFirstRun() const;
    QString getSettingsFilePath() const;

    // Validation
    bool isValidDirectory(const QString& dir) const;
    bool validateSettings();

    // Checkbox state management
    void initializeCheckbox(QCheckBox* checkbox, const QString& key, bool defaultValue = true);
    // Action state management
    void initializeAction(QAction* action, const QString& key, bool defaultValue = true);

    // Generic state management
    bool getToggleState(const QString& key, bool defaultValue = true);
    void setToggleState(const QString& key, bool state);
    void saveActionState(QAction* action, const QString& key);
    void saveCheckboxState(QCheckBox* checkbox, const QString& key);


signals:
    // void settingChanged(const QString& key);
    // void settingsReset();
    // void directoryChanged(const QString& key, const QString& newPath);

private:
    // Private constructor for singleton
    SettingsManager();
    ~SettingsManager();

    // Prevent copying
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    // Initialization
    void initializeDefaults();
    void migrateSettings();
    // void setupConnections();

    // Version management
    // QString getCurrentVersion() const;
    void updateVersion();

    // // Internal helpers
    // void emitChangeSignals(const QString& key);
    // bool writeSettingsFile();
    // bool readSettingsFile();

    // Member variables
    std::unique_ptr<QSettings> m_settings;
    mutable QMutex m_mutex;
    int m_maxRecentFiles;
    const QString m_currentVersion;

    // Settings keys
    static const QString KEY_VERSION;
    static const QString KEY_FIRST_RUN;
    static const QString KEY_SHOW_TIMESTAMPS;
    static const QString KEY_MEDIA_DIR;
    static const QString KEY_EXPORT_DIR;
    static const QString KEY_TRANSCRIPTS_DIR;
    static const QString KEY_RECENT_FILES;
    static const QString KEY_WINDOW_PREFIX;
    static const QString TOGGLE_PREFIX;
};

// Template implementation
template<typename T>
T SettingsManager::getValue(const QString& key, const T& defaultValue) {
    QMutexLocker locker(&m_mutex);
    return m_settings->value(key, defaultValue).template value<T>();
}

template<typename T>
void SettingsManager::setValue(const QString& key, const T& value) {
    QMutexLocker locker(&m_mutex);
    m_settings->setValue(key, QVariant::fromValue(value));
    m_settings->sync();
    // emitChangeSignals(key);
}

