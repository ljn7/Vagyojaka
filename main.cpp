#include "tool.h"
#include <QApplication>
#include<QSettings>

const int MaxLogLines = 10000;

QMutex logMutex;  // Global mutex for thread safety

static const QHash<QtMsgType, QString>& getMsgLevelHash() {
    static const QHash<QtMsgType, QString> hash({
        {QtDebugMsg, "Debug"},
        {QtInfoMsg, "Info"},
        {QtWarningMsg, "Warning"},
        {QtCriticalMsg, "Critical"},
        {QtFatalMsg, "Fatal"}
    });
    return hash;
}


void writeLogToFile(const QString &logText) {
    // Get the application-specific data directory
    QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";

    // Ensure the logs directory exists
    QDir dir;
    if (!dir.exists(logDirPath)) {
        dir.mkpath(logDirPath);
    }

    // Generate new log file name (once per session)
    static QString logFilePath = logDirPath + "/logfile-" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".log";

    logMutex.lock();  // Lock before accessing the file

    QFile outFile(logFilePath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream textStream(&outFile);
        textStream.setEncoding(QStringConverter::Utf8);
        textStream << logText << "\n";
    }

    logMutex.unlock();  // Unlock after writing
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString dateTime = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString dateTimeText = QString("[%1] ").arg(dateTime);

    const char *file = context.file ? context.file : "";

    QString logLevelName = getMsgLevelHash().value(type, "Unknown");
    QString logText = QString("%1 %2: %3 (%4)").arg(dateTimeText, logLevelName, msg, file);

    // Run log writing in a separate thread using QtConcurrent
    QtConcurrent::run(writeLogToFile, logText);
}

int main(int argc, char *argv[])
{

    qInstallMessageHandler(customMessageHandler);
    QApplication app(argc, argv);
    app.setApplicationName("Vagyojaka");
    // app.setApplicationDisplayName("Vagyojaka: ASR Post Editor");
    app.setOrganizationName("IIT Bombay");
    Tool w;
    w.show();

    return app.exec();
}

