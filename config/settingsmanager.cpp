// SettingsManager.cpp
#include "config/settingsmanager.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// Initialize static constants
const QString SettingsManager::KEY_VERSION = "General/Version";
const QString SettingsManager::KEY_FIRST_RUN = "General/FirstRun";
const QString SettingsManager::KEY_SHOW_TIMESTAMPS = "Interface/ShowTimeStamps";
const QString SettingsManager::TOGGLE_PREFIX = "UI/Toggles/";
const QString SettingsManager::KEY_MEDIA_DIR = "Directory/Media";
const QString SettingsManager::KEY_EXPORT_DIR = "Directory/Export";
const QString SettingsManager::KEY_TRANSCRIPTS_DIR = "Directories/Transcripts";
const QString SettingsManager::KEY_RECENT_FILES = "Files/RecentFiles";
const QString SettingsManager::KEY_WINDOW_PREFIX = "Windows/";

SettingsManager& SettingsManager::getInstance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager()
    : m_maxRecentFiles(10)
    , m_currentVersion("1.0.0") {

    QString iniPath = QApplication::applicationDirPath() +
                      QDir::separator() + "config.ini";
    m_settings = std::make_unique<QSettings>(iniPath, QSettings::IniFormat);
    if (isFirstRun()) {
        initializeDefaults();
        m_settings->setValue(KEY_FIRST_RUN, false);
    } else {
        migrateSettings();
    }

    // setupConnections();
}

SettingsManager::~SettingsManager() {
    m_settings->sync();
}

void SettingsManager::initializeDefaults() {
    QMutexLocker locker(&m_mutex);

    m_settings->setValue(KEY_VERSION, m_currentVersion);
    m_settings->setValue(KEY_SHOW_TIMESTAMPS, true);

    // Set default directories
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_settings->setValue(KEY_MEDIA_DIR, documentsPath + "/Media");
    m_settings->setValue(KEY_EXPORT_DIR, documentsPath + "/Exports");
    m_settings->setValue(KEY_TRANSCRIPTS_DIR, documentsPath + "/Transcripts");

    m_settings->sync();
}

// void SettingsManager::setupConnections() {
//     connect(this, &SettingsManager::settingChanged,
//             this, &SettingsManager::emitChangeSignals);
// }

// void SettingsManager::emitChangeSignals(const QString& key) {
//     if (key.startsWith("Directories/")) {
//         emit directoryChanged(key, m_settings->value(key).toString());
//     }
// }

bool SettingsManager::isFirstRun() const {
    return !m_settings->contains(KEY_FIRST_RUN) ||
           m_settings->value(KEY_FIRST_RUN, true).toBool();
}

void SettingsManager::migrateSettings() {
    QString currentVersion = m_settings->value(KEY_VERSION).toString();
    if (currentVersion != m_currentVersion) {
        // Implement version-specific migrations here
        updateVersion();
    }
}

void SettingsManager::updateVersion() {
    m_settings->setValue(KEY_VERSION, m_currentVersion);
    m_settings->sync();
}

// Interface settings implementations
bool SettingsManager::getShowTimeStamps() {
    return getValue<bool>(KEY_SHOW_TIMESTAMPS, true);
}

void SettingsManager::setShowTimeStamps(bool show) {
    setValue(KEY_SHOW_TIMESTAMPS, show);
}


// Directory handling implementations
QString SettingsManager::getMediaDirectory() {
    return getValue<QString>(KEY_MEDIA_DIR,
                             QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
}

void SettingsManager::setMediaDirectory(const QString& dir) {
    if (isValidDirectory(dir)) {
        setValue(KEY_MEDIA_DIR, dir);
    }
}

QString SettingsManager::getTranscriptsDirectory() {
    return getValue<QString>(KEY_TRANSCRIPTS_DIR, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
}

void SettingsManager::setTranscriptsDirectory(const QString& dir) {
    if (isValidDirectory(dir)) {
        setValue(KEY_TRANSCRIPTS_DIR, dir);
    }
}

bool SettingsManager::isValidDirectory(const QString& dir) const {
    QDir directory(dir);
    return directory.exists() && directory.isReadable();
}

// // Window state implementations
// void SettingsManager::saveWindowGeometry(const QString& windowName, const QRect& geometry) {
//     setValue(KEY_WINDOW_PREFIX + windowName + "/Geometry", geometry);
// }

// QRect SettingsManager::loadWindowGeometry(const QString& windowName) {
//     return getValue<QRect>(KEY_WINDOW_PREFIX + windowName + "/Geometry");
// }

// // Recent files implementations
// void SettingsManager::addRecentFile(const QString& filePath) {
//     QMutexLocker locker(&m_mutex);
//     QStringList files = m_settings->value(KEY_RECENT_FILES).toStringList();
//     files.removeAll(filePath);
//     files.prepend(filePath);

//     while (files.size() > m_maxRecentFiles) {
//         files.removeLast();
//     }

//     m_settings->setValue(KEY_RECENT_FILES, files);
//     m_settings->sync();
//     emit settingChanged(KEY_RECENT_FILES);
// }

// QStringList SettingsManager::getRecentFiles() {
//     return getValue<QStringList>(KEY_RECENT_FILES);
// }

// void SettingsManager::clearRecentFiles() {
//     setValue(KEY_RECENT_FILES, QStringList());
// }

// // Settings management implementations
// bool SettingsManager::backup(const QString& backupPath) {
//     QMutexLocker locker(&m_mutex);
//     try {
//         QJsonObject root;
//         for (const QString& key : m_settings->allKeys()) {
//             root[key] = QJsonValue::fromVariant(m_settings->value(key));
//         }

//         QFile backupFile(backupPath);
//         if (!backupFile.open(QIODevice::WriteOnly)) {
//             return false;
//         }

//         QJsonDocument doc(root);
//         backupFile.write(doc.toJson());
//         return true;
//     } catch (...) {
//         qDebug() << "Failed to backup settings";
//         return false;
//     }
// }

void SettingsManager::resetToDefaults() {
    QMutexLocker locker(&m_mutex);
    m_settings->clear();
    initializeDefaults();
    // emit settingsReset();
}

QString SettingsManager::getSettingsFilePath() const {
    return m_settings->fileName();
}

void SettingsManager::initializeAction(QAction* action, const QString& key, bool defaultValue) {
    if (!action) return;
    bool state;
    {
        QMutexLocker locker(&m_mutex);
        QString fullKey = TOGGLE_PREFIX + key;
        QString savedValue = m_settings->value(fullKey).toString();
        if (savedValue.isEmpty()) {
            state = defaultValue;
            m_settings->setValue(fullKey, defaultValue ? "true" : "false");
            m_settings->sync();
        } else {
            state = (savedValue == "true");
        }
    }
    action->setChecked(state);
}

void SettingsManager::initializeCheckbox(QCheckBox* checkbox, const QString& key, bool defaultValue) {
    if (!checkbox) return;

    bool state;
    {
        QMutexLocker locker(&m_mutex);
        QString fullKey = TOGGLE_PREFIX + key;
        QString savedValue = m_settings->value(fullKey).toString();

        if (savedValue.isEmpty()) {
            state = defaultValue;
            // Save the default value without recursive mutex lock
            m_settings->setValue(fullKey, defaultValue ? "true" : "false");
            m_settings->sync();
        } else {
            state = (savedValue == "true");
        }
    }

    checkbox->setChecked(state);
}

bool SettingsManager::getToggleState(const QString& key, bool defaultValue) {
    QMutexLocker locker(&m_mutex);
    QString fullKey = TOGGLE_PREFIX + key;
    QString value = m_settings->value(fullKey).toString();

    if (value.isEmpty()) {
        return defaultValue;
    }
    return value == "true";
}

void SettingsManager::setToggleState(const QString& key, bool state) {
    QMutexLocker locker(&m_mutex);
    QString fullKey = TOGGLE_PREFIX + key;
    m_settings->setValue(fullKey, state ? "true" : "false");
    m_settings->sync();
    // emit settingChanged(fullKey);
}

void SettingsManager::saveActionState(QAction* action, const QString& key) {
    if (action) {
        setToggleState(key, action->isChecked());
    }
}

void SettingsManager::saveCheckboxState(QCheckBox* checkbox, const QString& key) {
    if (checkbox) {
        setToggleState(key, checkbox->isChecked());
    }
}
