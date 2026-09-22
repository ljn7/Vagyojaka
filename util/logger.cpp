#include "util/logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGlobal>

#include <cstdlib>

namespace {

/*! Roll over to a new file once the current one reaches this many lines. */
constexpr int MaxLogLines = 10000;

/*! Flush this often. Anything at warning level or above flushes immediately anyway. */
constexpr int FlushEveryLines = 64;

/*! Number of previous session logs to keep when pruning at startup. */
constexpr int MaxSessionsKept = 20;

QMutex g_mutex;
QFile g_file;
QTextStream g_stream;
int g_lineCount = 0;
int g_sinceFlush = 0;
int g_rotation = 0;
QtMessageHandler g_previousHandler = nullptr;
bool g_installed = false;

/*!
 * \brief Guards against a log write that itself logs.
 *
 * QMutex is not recursive, so without this a warning raised from inside the write path
 * (a failing QFile, for instance) would deadlock the thread that emitted it.
 */
thread_local bool t_inHandler = false;

QString logDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs");
}

QString levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("Debug");
    case QtInfoMsg:
        return QStringLiteral("Info");
    case QtWarningMsg:
        return QStringLiteral("Warning");
    case QtCriticalMsg:
        return QStringLiteral("Critical");
    case QtFatalMsg:
        return QStringLiteral("Fatal");
    }
    return QStringLiteral("Unknown");
}

/*! \brief Deletes all but the newest MaxSessionsKept log files. */
void pruneOldLogs(const QDir& dir)
{
    QFileInfoList logs = dir.entryInfoList({QStringLiteral("logfile-*.log")}, QDir::Files, QDir::Time);
    for (int i = MaxSessionsKept; i < logs.size(); ++i)
        QFile::remove(logs.at(i).absoluteFilePath());
}

/*! \brief Opens a log file. The caller holds the lock. */
bool openLogFile(int rotation)
{
    const QString dirPath = logDirectory();
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return false;

    static const QString session = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));

    QString name = QStringLiteral("%1/logfile-%2").arg(dirPath, session);
    if (rotation > 0)
        name += QStringLiteral(".%1").arg(rotation);
    name += QStringLiteral(".log");

    g_file.setFileName(name);
    if (!g_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return false;

    g_stream.setDevice(&g_file);
    g_stream.setEncoding(QStringConverter::Utf8);
    g_lineCount = 0;
    g_sinceFlush = 0;

    if (rotation == 0)
        pruneOldLogs(dir);

    return true;
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (t_inHandler)
        return;

    t_inHandler = true;

    const QString line = QStringLiteral("[%1] %2: %3 (%4)")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm:ss")),
                                  levelName(type), msg, QString::fromUtf8(context.file ? context.file : ""));

    {
        QMutexLocker locker(&g_mutex);

        if (g_file.isOpen()) {
            if (g_lineCount >= MaxLogLines) {
                g_stream.flush();
                g_stream.setDevice(nullptr);
                g_file.close();
                openLogFile(++g_rotation);
            }

            if (g_file.isOpen()) {
                g_stream << line << '\n';
                ++g_lineCount;

                // Anything a user would investigate after a crash is flushed at once.
                // Routine debug output rides along in the buffer.
                if (++g_sinceFlush >= FlushEveryLines || type >= QtWarningMsg) {
                    g_stream.flush();
                    g_sinceFlush = 0;
                }
            }
        }
    }

    t_inHandler = false;

    // QtFatalMsg must not return. Hand it to the default handler, which aborts.
    if (type == QtFatalMsg) {
        Logger::shutdown();
        if (g_previousHandler)
            g_previousHandler(type, context, msg);
        else
            abort();
    }
}

} // namespace

namespace Logger {

void install()
{
    QMutexLocker locker(&g_mutex);
    if (g_installed)
        return;

    g_rotation = 0;
    if (!openLogFile(0)) {
        // Without a file there is nothing useful to do here, so leave Qt's default
        // handler in place rather than swallowing every message.
        return;
    }

    g_previousHandler = qInstallMessageHandler(messageHandler);
    g_installed = true;

    qAddPostRoutine(shutdown);
}

void shutdown()
{
    QMutexLocker locker(&g_mutex);
    if (!g_installed)
        return;

    qInstallMessageHandler(g_previousHandler);
    g_previousHandler = nullptr;
    g_installed = false;

    if (g_file.isOpen()) {
        g_stream.flush();
        g_stream.setDevice(nullptr);
        g_file.close();
    }
}

} // namespace Logger
