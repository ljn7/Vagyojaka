#pragma once

/*!
 * \brief Session logging for the application.
 *
 * Replaces the previous message handler, which dispatched a QtConcurrent task per
 * message and had that task open, append to and close the log file while holding a
 * global mutex. That meant a thread pool job, a lock and three syscalls for every
 * qDebug() line, discarded futures nothing ever waited on, and a recursion hazard if
 * anything inside the write path logged.
 *
 * This version keeps one buffered file open for the session, writes under a short lock
 * on the calling thread, flushes periodically and on anything at warning level or
 * above, rotates when the file grows past a line limit, and prunes old sessions.
 */
namespace Logger {

/*!
 * \brief Opens the session log and installs the Qt message handler.
 *
 * Safe to call once, early in main(), before the QApplication is constructed.
 */
void install();

/*!
 * \brief Flushes and closes the log, and restores the default message handler.
 *
 * Called automatically at exit. Calling it explicitly is harmless.
 */
void shutdown();

} // namespace Logger
