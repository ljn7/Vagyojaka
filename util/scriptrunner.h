#pragma once

#include <QString>
#include <QStringList>

class QWidget;

/*!
 * \brief Runs the bundled Python helper scripts without going through a shell.
 *
 * This replaces the old \c system() calls. Arguments are handed to QProcess as a
 * list, so a path containing spaces, quotes or shell metacharacters can no longer
 * be interpreted as a command. The interpreter is looked up on PATH rather than
 * assumed to be \c python3, which also makes the helpers work on Windows.
 *
 * The caller blocks until the script finishes, but the event loop keeps running
 * behind a modal progress dialog, so the window still repaints and the user can
 * cancel. The wait is always bounded by a timeout.
 */
namespace ScriptRunner {

/*!
 * \brief Outcome of a script run.
 */
struct Result
{
    bool ok = false;   ///< True only when the process started and exited with code 0.
    int exitCode = -1; ///< Process exit code, or -1 when it never exited normally.
    QString stdOut;    ///< Everything the script wrote to standard output.
    QString stdErr;    ///< Everything the script wrote to standard error.
    QString error;     ///< Human readable reason, set whenever \c ok is false.
};

/*!
 * \brief Locates a Python 3 interpreter.
 *
 * The result is cached after the first call.
 *
 * \return The program followed by any leading arguments it needs (the Windows
 *         launcher needs \c -3), or an empty list when no interpreter was found.
 */
QStringList pythonCommand();

/*!
 * \brief Copies a bundled Qt resource next to the application if it is not there yet.
 *
 * \param resourcePath Qt resource path, for example ":/alignment.py".
 * \param targetPath   Destination on disk.
 * \param errorOut     Optional; receives the failure reason.
 * \return True when the file exists on disk afterwards.
 */
bool ensureExtracted(const QString& resourcePath, const QString& targetPath, QString* errorOut = nullptr);

/*!
 * \brief Runs a Python script and waits for it to finish.
 *
 * \param scriptPath    Path to the script. Passed to the interpreter as an argument.
 * \param arguments     Script arguments. Each element is passed verbatim, no quoting needed.
 * \param parent        Parent for the progress dialog. May be null.
 * \param progressLabel Text shown in the progress dialog.
 * \param timeoutMs     Upper bound on the wait. The process is killed when it elapses.
 */
Result runPython(const QString& scriptPath, const QStringList& arguments, QWidget* parent = nullptr,
                 const QString& progressLabel = QString(), int timeoutMs = 120000);

} // namespace ScriptRunner
