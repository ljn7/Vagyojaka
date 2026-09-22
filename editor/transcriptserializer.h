#pragma once

#include "blockandword.h"

#include <QString>
#include <QTime>
#include <QVector>

class QIODevice;

/*!
 * \brief Reading and writing of transcript XML.
 *
 * Extracted from Editor, which previously parsed and serialised the document format
 * itself. Keeping it here means the format can be exercised without constructing a
 * text widget, and the editor no longer has to know what a QXmlStreamReader is.
 */
namespace TranscriptSerializer {

/*! \brief A parsed transcript. */
struct Transcript
{
    QVector<block> blocks;
    QString language;
};

/*!
 * \brief Parses a transcript document.
 *
 * Unknown elements are skipped rather than treated as errors, matching the previous
 * behaviour.
 *
 * \param device  An open, readable device.
 * \param errorOut Optional; receives a parse error message.
 */
Transcript read(QIODevice& device, QString* errorOut = nullptr);

/*!
 * \brief Writes blocks as a transcript document.
 *
 * Blocks with empty text are skipped, as they always were. The device is neither
 * closed nor deleted here: the old saveXml() did both to a QFile it did not own,
 * which made the ownership impossible to follow at the call sites.
 */
void write(QIODevice& device, const QVector<block>& blocks, const QString& language);

/*!
 * \brief Parses a timestamp.
 *
 * Accepts "h:m:s.z" and "m:s.z", with or without the fractional part.
 */
QTime parseTime(const QString& text);

/*!
 * \brief Normalises the timestamp forms found in older transcripts.
 *
 * Some files carry minute counts above 59 or hour counts that need carrying over.
 * Returns an invalid QTime when the text cannot be interpreted at all.
 */
QTime parseTimeLenient(const QString& text);

/*! \brief Builds a word, normalising the isEdited flag. */
word makeWord(const QTime& timeStamp, const QString& text, const QStringList& tagList,
              const QString& isEdited);

} // namespace TranscriptSerializer
