// Round trip and parsing checks for the transcript XML format.
//
// This matters more than most tests here: a change that quietly alters the format
// corrupts the user's saved work, and the damage is only visible the next time they
// open the file.
#include "editor/transcriptserializer.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>

static int g_failures = 0;

static void check(bool ok, const QString& what)
{
    if (!ok) {
        qWarning().noquote() << "FAIL:" << what;
        ++g_failures;
    }
}

static QVector<block> sampleBlocks()
{
    QVector<block> blocks;

    blocks.append(block(QTime(0, 0, 1, 500), "hello world", "SPEAKER_A", {"greeting"},
                        {word(QTime(0, 0, 1, 500), "hello", {}, "false"),
                         word(QTime(0, 0, 2, 0), "world", {"noun"}, "true")}));

    // Characters that must survive XML escaping. Note that block text is the words
    // joined by single spaces: the format stores words, and read() derives the text
    // from them, so a fixture whose text disagrees with its words cannot round trip.
    blocks.append(block(QTime(1, 2, 3, 4), "a<b &c> \"quoted\"", "SPEAKER_B", {},
                        {word(QTime(1, 2, 3, 4), "a<b", {}, "false"),
                         word(QTime(1, 2, 3, 5), "&c>", {}, "false"),
                         word(QTime(1, 2, 3, 6), "\"quoted\"", {}, "false")}));

    blocks.append(block(QTime(0, 10, 0, 0), QString::fromUtf8("नमस्ते दुनिया"), "वक्ता", {"hindi"},
                        {word(QTime(0, 10, 0, 0), QString::fromUtf8("नमस्ते"), {}, "false"),
                         word(QTime(0, 10, 1, 0), QString::fromUtf8("दुनिया"), {}, "false")}));

    return blocks;
}

static void testRoundTrip()
{
    const QVector<block> original = sampleBlocks();

    QBuffer out;
    out.open(QIODevice::WriteOnly);
    TranscriptSerializer::write(out, original, QStringLiteral("hindi"));
    out.close();

    QBuffer in(&out.buffer());
    in.open(QIODevice::ReadOnly);
    QString error;
    const auto parsed = TranscriptSerializer::read(in, &error);
    in.close();

    check(error.isEmpty(), QStringLiteral("round trip parses without error (%1)").arg(error));
    check(parsed.language == QStringLiteral("hindi"), "language survives the round trip");
    check(parsed.blocks.size() == original.size(),
          QStringLiteral("block count survives: %1 vs %2").arg(parsed.blocks.size()).arg(original.size()));

    for (qsizetype i = 0; i < qMin(parsed.blocks.size(), original.size()); ++i) {
        const block& a = original.at(i);
        const block& b = parsed.blocks.at(i);

        check(a.speaker == b.speaker, QStringLiteral("block %1 speaker").arg(i));
        check(a.timeStamp == b.timeStamp, QStringLiteral("block %1 timestamp").arg(i));
        check(a.text == b.text, QStringLiteral("block %1 text: %2 vs %3").arg(i).arg(a.text, b.text));
        check(a.tagList == b.tagList, QStringLiteral("block %1 tags").arg(i));
        check(a.words.size() == b.words.size(), QStringLiteral("block %1 word count").arg(i));

        for (qsizetype j = 0; j < qMin(a.words.size(), b.words.size()); ++j) {
            check(a.words.at(j).text == b.words.at(j).text, QStringLiteral("block %1 word %2 text").arg(i).arg(j));
            check(a.words.at(j).timeStamp == b.words.at(j).timeStamp,
                  QStringLiteral("block %1 word %2 timestamp").arg(i).arg(j));
            check(a.words.at(j).isEdited == b.words.at(j).isEdited,
                  QStringLiteral("block %1 word %2 isEdited").arg(i).arg(j));
            check(a.words.at(j).tagList == b.words.at(j).tagList,
                  QStringLiteral("block %1 word %2 tags").arg(i).arg(j));
        }

        // operator== ignores tagList by design, so this must hold too.
        check(a == b, QStringLiteral("block %1 compares equal").arg(i));
    }
}

// The format stores words and derives block text from them. Anything that changes
// that relationship silently rewrites the user's lines, so state it as a check.
static void testTextIsDerivedFromWords()
{
    QVector<block> blocks;
    blocks.append(block(QTime(0, 0, 1, 0), "this text will be ignored on read", "A", {},
                        {word(QTime(0, 0, 1, 0), "actual", {}, "false"),
                         word(QTime(0, 0, 2, 0), "words", {}, "false")}));

    QBuffer out;
    out.open(QIODevice::WriteOnly);
    TranscriptSerializer::write(out, blocks, QStringLiteral("english"));
    out.close();

    QBuffer in(&out.buffer());
    in.open(QIODevice::ReadOnly);
    const auto parsed = TranscriptSerializer::read(in);
    in.close();

    check(parsed.blocks.size() == 1, "derived text: one block");
    if (!parsed.blocks.isEmpty())
        check(parsed.blocks.first().text == QStringLiteral("actual words"),
              QStringLiteral("block text is rebuilt from words, got: %1").arg(parsed.blocks.first().text));
}

static void testTimeParsing()
{
    using namespace TranscriptSerializer;

    check(parseTime("01:02:03.004") == QTime(1, 2, 3, 4), "h:m:s.z");
    check(parseTime("02:03.004") == QTime(0, 2, 3, 4), "m:s.z");
    check(parseTime("01:02:03") == QTime(1, 2, 3, 0), "h:m:s");
    check(parseTime("02:03") == QTime(0, 2, 3, 0), "m:s");
    check(!parseTime("nonsense").isValid(), "garbage is not a time");

    // Minute counts that overflow their field, as older transcripts contain.
    check(parseTimeLenient("75:30") == QTime(1, 15, 30, 0), "lenient m:s with minute overflow");
    check(parseTimeLenient("00:75:30") == QTime(1, 15, 30, 0), "lenient h:m:s with minute overflow");
    check(parseTimeLenient("01:02:03.004") == QTime(1, 2, 3, 4), "lenient passes valid input through");
    check(!parseTimeLenient("nonsense").isValid(), "lenient rejects garbage");
}

static void testMalformedInput()
{
    const QByteArray notATranscript = "<?xml version=\"1.0\"?><something><else/></something>";
    QByteArray data(notATranscript);
    QBuffer in(&data);
    in.open(QIODevice::ReadOnly);
    QString error;
    const auto parsed = TranscriptSerializer::read(in, &error);
    in.close();

    check(parsed.blocks.isEmpty(), "wrong root element yields no blocks");
    check(!error.isEmpty(), "wrong root element reports an error");

    // Unknown elements inside a transcript are skipped rather than rejected.
    QByteArray mixed =
        "<?xml version=\"1.0\"?><transcript lang=\"english\">"
        "<note>ignored</note>"
        "<line timestamp=\"00:00:01.000\" speaker=\"A\"><word timestamp=\"00:00:01.000\">hi</word></line>"
        "</transcript>";
    QBuffer in2(&mixed);
    in2.open(QIODevice::ReadOnly);
    QString error2;
    const auto parsed2 = TranscriptSerializer::read(in2, &error2);
    in2.close();

    check(parsed2.blocks.size() == 1, "unknown elements are skipped");
    check(parsed2.language == QStringLiteral("english"), "language attribute is read");
    if (!parsed2.blocks.isEmpty())
        check(parsed2.blocks.first().text == QStringLiteral("hi"), "text of the surviving line");
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    testRoundTrip();
    testTextIsDerivedFromWords();
    testTimeParsing();
    testMalformedInput();

    if (g_failures == 0) {
        qInfo().noquote() << "ALL SERIALIZER CHECKS PASSED";
        return 0;
    }
    qWarning().noquote() << g_failures << "CHECK(S) FAILED";
    return 1;
}
