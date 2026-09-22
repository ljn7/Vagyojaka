#pragma once

#include "blockandword.h"

#include <QString>
#include <QVector>

/*!
 * \brief Renders a transcript to the formats the editor can export.
 *
 * Extracted from Editor. The rendering is pure: it takes blocks and returns text, so
 * the output format can be checked without a widget, a printer or a file dialog. The
 * editor keeps the dialogs and passes the chosen path in.
 */
namespace TranscriptExporter {

/*!
 * \brief Renders the transcript as an HTML document, for PDF output.
 *
 * \param includeTimeStamps Appends each line's timestamp when true.
 */
QString toHtml(const QVector<block>& blocks, bool includeTimeStamps);

/*! \brief Renders the transcript as plain text, always including timestamps. */
QString toPlainText(const QVector<block>& blocks);

/*!
 * \brief Writes \a html to an A4 PDF at \a path.
 * \param errorOut Optional; receives the failure reason.
 */
bool writePdf(const QString& path, const QString& html, QString* errorOut = nullptr);

/*!
 * \brief Writes \a text to \a path as UTF-8.
 * \param errorOut Optional; receives the failure reason.
 */
bool writeText(const QString& path, const QString& text, QString* errorOut = nullptr);

} // namespace TranscriptExporter
