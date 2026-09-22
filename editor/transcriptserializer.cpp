#include "transcriptserializer.h"

#include <QCoreApplication>
#include <QIODevice>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <utility>

namespace {
constexpr auto TimeStampFormat = "hh:mm:ss.zzz";
}

namespace TranscriptSerializer {

QTime parseTime(const QString& text)
{
    if (text.contains(".")) {
        if (text.count(":") == 2)
            return QTime::fromString(text, "h:m:s.z");
        return QTime::fromString(text, "m:s.z");
    }

    if (text.count(":") == 2)
        return QTime::fromString(text, "h:m:s");
    return QTime::fromString(text, "m:s");
}

QTime parseTimeLenient(const QString& text)
{
    QTime parsed = parseTime(text);
    if (parsed.isValid())
        return parsed;

    // Older transcripts store minute or second counts that overflow their field, for
    // example "75:30" meaning an hour and a quarter. Carry the overflow up by hand.
    const QStringList parts = text.split(":");

    if (parts.size() == 2) {
        const int totalMinutes = parts.at(0).toInt();
        const QString rebuilt = QStringLiteral("%1:%2:%3")
                                    .arg(totalMinutes / 60, 2, 10, QLatin1Char('0'))
                                    .arg(totalMinutes % 60)
                                    .arg(parts.at(1));
        return parseTime(rebuilt);
    }

    if (parts.size() == 3) {
        const int hours = (parts.at(1).toInt() / 60) + parts.at(0).toInt();
        const QString rebuilt = QStringLiteral("%1:%2:%3")
                                    .arg(hours, 2, 10, QLatin1Char('0'))
                                    .arg(parts.at(1).toInt() % 60)
                                    .arg(parts.at(2));
        return parseTime(rebuilt);
    }

    return {};
}

word makeWord(const QTime& timeStamp, const QString& text, const QStringList& tagList,
              const QString& isEdited)
{
    return word(timeStamp, text, tagList, isEdited);
}

Transcript read(QIODevice& device, QString* errorOut)
{
    Transcript transcript;
    QXmlStreamReader reader(&device);

    if (!reader.readNextStartElement())
        return transcript;

    if (reader.name() != QString("transcript")) {
        if (errorOut)
            *errorOut = QCoreApplication::translate("TranscriptSerializer", "Incorrect file");
        return transcript;
    }

    transcript.language = reader.attributes().value("lang").toString();

    while (reader.readNextStartElement()) {
        if (reader.name() != QString("line")) {
            reader.skipCurrentElement();
            continue;
        }

        const QTime blockTimeStamp = parseTimeLenient(reader.attributes().value("timestamp").toString());
        const QString blockSpeaker = reader.attributes().value("speaker").toString();
        const QString tagString = reader.attributes().value("tags").toString();

        QStringList tagList;
        if (!tagString.isEmpty())
            tagList = tagString.split(",");

        block line(blockTimeStamp, QString(), blockSpeaker, tagList, QVector<word>());
        QString blockText;

        while (reader.readNextStartElement()) {
            if (reader.name() != QString("word")) {
                reader.skipCurrentElement();
                continue;
            }

            const QString isEditedStr = reader.attributes().value("isEdited").toString();
            const QTime wordTimeStamp = parseTime(reader.attributes().value("timestamp").toString());
            const QString wordTagString = reader.attributes().value("tags").toString();
            const QString wordText = reader.readElementText();

            QStringList wordTagList;
            if (!wordTagString.isEmpty())
                wordTagList = wordTagString.split(",");

            blockText += wordText + QLatin1Char(' ');
            line.words.append(makeWord(wordTimeStamp, wordText, wordTagList, isEditedStr.toLower()));
        }

        line.text = blockText.trimmed();
        transcript.blocks.append(line);
    }

    if (reader.hasError() && errorOut)
        *errorOut = reader.errorString();

    return transcript;
}

void write(QIODevice& device, const QVector<block>& blocks, const QString& language)
{
    QXmlStreamWriter writer(&device);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("transcript");

    if (!language.isEmpty())
        writer.writeAttribute("lang", language);

    for (const block& a_block : std::as_const(blocks)) {
        if (a_block.text.isEmpty())
            continue;

        writer.writeStartElement("line");
        writer.writeAttribute("timestamp", a_block.timeStamp.toString(TimeStampFormat));
        writer.writeAttribute("speaker", a_block.speaker);

        if (!a_block.tagList.isEmpty())
            writer.writeAttribute("tags", a_block.tagList.join(","));

        for (const word& a_word : std::as_const(a_block.words)) {
            writer.writeStartElement("word");
            writer.writeAttribute("timestamp", a_word.timeStamp.toString(TimeStampFormat));
            writer.writeAttribute("isEdited", a_word.isEdited == "true" ? "true" : "false");

            if (!a_word.tagList.isEmpty())
                writer.writeAttribute("tags", a_word.tagList.join(","));

            writer.writeCharacters(a_word.text);
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
}

} // namespace TranscriptSerializer
