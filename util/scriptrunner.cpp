#include "util/scriptrunner.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTimer>

#include <memory>
#include <utility>

namespace {

Q_LOGGING_CATEGORY(lcScript, "vagyojaka.script")

/*!
 * \brief Confirms that a candidate interpreter really is Python 3.
 *
 * On several distributions "python" is still Python 2, and on Windows a stub
 * "python.exe" from the Microsoft Store opens a store page instead of running.
 */
bool isPython3(const QString& program, const QStringList& leadingArgs)
{
    QProcess probe;
    probe.start(program, leadingArgs + QStringList{"--version"});
    if (!probe.waitForFinished(5000) || probe.exitStatus() != QProcess::NormalExit
        || probe.exitCode() != 0) {
        return false;
    }

    // Python prints the version to stdout on 3.4 and newer, to stderr before that.
    const QString output = QString::fromUtf8(probe.readAllStandardOutput())
                           + QString::fromUtf8(probe.readAllStandardError());
    return output.contains(QStringLiteral("Python 3"));
}

} // namespace

namespace ScriptRunner {

QStringList pythonCommand()
{
    static const QStringList cached = [] {
        struct Candidate
        {
            QString program;
            QStringList leadingArgs;
        };

        QList<Candidate> candidates{{QStringLiteral("python3"), {}}, {QStringLiteral("python"), {}}};
#ifdef Q_OS_WIN
        candidates.append({QStringLiteral("py"), {QStringLiteral("-3")}});
#endif

        for (const Candidate& candidate : std::as_const(candidates)) {
            const QString resolved = QStandardPaths::findExecutable(candidate.program);
            if (resolved.isEmpty())
                continue;
            if (!isPython3(resolved, candidate.leadingArgs))
                continue;

            qCInfo(lcScript) << "Using Python interpreter" << resolved;
            return QStringList{resolved} + candidate.leadingArgs;
        }

        qCWarning(lcScript) << "No Python 3 interpreter found on PATH";
        return QStringList{};
    }();

    return cached;
}

bool ensureExtracted(const QString& resourcePath, const QString& targetPath, QString* errorOut)
{
    const auto fail = [errorOut](const QString& reason) {
        if (errorOut)
            *errorOut = reason;
        qCWarning(lcScript) << reason;
        return false;
    };

    if (QFile::exists(targetPath))
        return true;

    QFile source(resourcePath);
    if (!source.open(QIODevice::ReadOnly))
        return fail(QObject::tr("Could not read bundled script %1: %2").arg(resourcePath, source.errorString()));

    const QDir targetDir = QFileInfo(targetPath).absoluteDir();
    if (!targetDir.exists() && !targetDir.mkpath(QStringLiteral(".")))
        return fail(QObject::tr("Could not create %1").arg(targetDir.absolutePath()));

    QFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return fail(QObject::tr("Could not write %1: %2").arg(targetPath, target.errorString()));

    if (target.write(source.readAll()) < 0)
        return fail(QObject::tr("Could not write %1: %2").arg(targetPath, target.errorString()));

    // No chmod is needed. The interpreter is invoked explicitly, so the script
    // never has to be executable on its own.
    return true;
}

Result runPython(const QString& scriptPath, const QStringList& arguments, QWidget* parent,
                 const QString& progressLabel, int timeoutMs)
{
    Result result;

    const QStringList interpreter = pythonCommand();
    if (interpreter.isEmpty()) {
        result.error = QObject::tr("No Python 3 interpreter was found on PATH. "
                                   "Install Python 3 and make sure it is on PATH.");
        return result;
    }

    if (!QFile::exists(scriptPath)) {
        result.error = QObject::tr("Script not found: %1").arg(scriptPath);
        return result;
    }

    const QString program = interpreter.first();
    QStringList args = interpreter.mid(1);
    args << QFileInfo(scriptPath).absoluteFilePath();
    args << arguments;

    QProcess process;
    process.setProgram(program);
    process.setArguments(args);

    QEventLoop loop;
    bool timedOut = false;
    bool canceled = false;

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        timedOut = true;
        process.kill();
    });

    QObject::connect(&process, &QProcess::finished, &loop, [&loop] { loop.quit(); });
    QObject::connect(&process, &QProcess::errorOccurred, &loop, [&loop] { loop.quit(); });

    // A modal dialog keeps the window painting and, just as importantly, stops the
    // user from re-entering the action that started this process.
    std::unique_ptr<QProgressDialog> dialog;
    if (qobject_cast<QApplication*>(QCoreApplication::instance())) {
        dialog = std::make_unique<QProgressDialog>(
            progressLabel.isEmpty() ? QObject::tr("Running %1...").arg(QFileInfo(scriptPath).fileName())
                                    : progressLabel,
            QObject::tr("Cancel"), 0, 0, parent);
        dialog->setWindowModality(Qt::ApplicationModal);
        dialog->setMinimumDuration(400);
        dialog->setAutoClose(false);
        dialog->setAutoReset(false);
        QObject::connect(dialog.get(), &QProgressDialog::canceled, &loop, [&] {
            canceled = true;
            process.kill();
        });
    }

    qCInfo(lcScript) << "Running" << program << args;
    process.start();

    if (!process.waitForStarted(10000)) {
        result.error = QObject::tr("Could not start %1: %2").arg(program, process.errorString());
        return result;
    }

    timeout.start(timeoutMs);
    loop.exec();
    timeout.stop();

    if (process.state() != QProcess::NotRunning)
        process.waitForFinished(2000);

    result.stdOut = QString::fromUtf8(process.readAllStandardOutput());
    result.stdErr = QString::fromUtf8(process.readAllStandardError());
    result.exitCode = process.exitCode();

    if (canceled) {
        result.error = QObject::tr("Canceled.");
    }
    else if (timedOut) {
        result.error = QObject::tr("%1 did not finish within %2 seconds and was stopped.")
                           .arg(QFileInfo(scriptPath).fileName())
                           .arg(timeoutMs / 1000);
    }
    else if (process.error() == QProcess::FailedToStart) {
        result.error = QObject::tr("Could not start %1: %2").arg(program, process.errorString());
    }
    else if (process.exitStatus() != QProcess::NormalExit) {
        result.error = QObject::tr("%1 crashed.").arg(QFileInfo(scriptPath).fileName());
    }
    else if (result.exitCode != 0) {
        result.error = QObject::tr("%1 failed with exit code %2.\n%3")
                           .arg(QFileInfo(scriptPath).fileName())
                           .arg(result.exitCode)
                           .arg(result.stdErr.trimmed());
    }
    else {
        result.ok = true;
    }

    if (!result.ok)
        qCWarning(lcScript) << "Script run failed:" << result.error;

    return result;
}

} // namespace ScriptRunner
