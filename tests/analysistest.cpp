// Checks for the word classification that drives spell check underlining.
//
// This logic existed in four copies inside Editor and they disagreed about how to
// normalise a word before looking it up. These checks pin the unified behaviour down.
#include "editor/transcriptanalysis.h"

#include <QCoreApplication>
#include <QDebug>

using namespace TranscriptAnalysis;

static const QString Punctuation = QStringLiteral(",.!;:?");

static int g_failures = 0;

static void check(bool ok, const QString& what)
{
    if (!ok) {
        qWarning().noquote() << "FAIL:" << what;
        ++g_failures;
    }
}

static void checkEq(const QString& got, const QString& expected, const QString& what)
{
    check(got == expected, QStringLiteral("%1 (got \"%2\", expected \"%3\")").arg(what, got, expected));
}

// ------------------------------------------------------------------- normalizeWord
static void testNormalizeWord()
{
    checkEq(normalizeWord("Hello", Punctuation), "hello", "lowercases");
    checkEq(normalizeWord("hello.", Punctuation), "hello", "drops a trailing full stop");
    checkEq(normalizeWord("hello,", Punctuation), "hello", "drops a trailing comma");
    checkEq(normalizeWord("hello?", Punctuation), "hello", "drops a trailing question mark");

    // Paired delimiters, which two of the four original copies did not handle at all.
    checkEq(normalizeWord("\"hello\"", Punctuation), "hello", "peels double quotes");
    checkEq(normalizeWord("(hello)", Punctuation), "hello", "peels parentheses");
    checkEq(normalizeWord("[hello]", Punctuation), "hello", "peels square brackets");
    checkEq(normalizeWord("{hello}", Punctuation), "hello", "peels braces");
    checkEq(normalizeWord("'hello'", Punctuation), "hello", "peels apostrophes");
    checkEq(normalizeWord("<hello>", Punctuation), "hello", "peels angle brackets");

    // Punctuation stripping runs before delimiter peeling, and the extra trailing
    // pass afterwards is what lets both of these end up at the same place.
    checkEq(normalizeWord("hello)?", Punctuation), "hello", "trailing mark then closing bracket");
    checkEq(normalizeWord("hello?)", Punctuation), "hello", "closing bracket then trailing mark");

    checkEq(normalizeWord("(hello),", Punctuation), "hello", "bracketed word with a trailing comma");

    // Only one delimiter of each kind is peeled, and an unmatched one still goes.
    checkEq(normalizeWord("(hello", Punctuation), "hello", "unmatched opening bracket");
    checkEq(normalizeWord("hello)", Punctuation), "hello", "unmatched closing bracket");

    checkEq(normalizeWord("", Punctuation), "", "empty input stays empty");
    checkEq(normalizeWord(".", Punctuation), "", "a lone punctuation mark reduces to nothing");
    checkEq(normalizeWord("()", Punctuation), "", "a lone bracket pair reduces to nothing");

    // Non-Latin text must be left alone.
    checkEq(normalizeWord(QString::fromUtf8("नमस्ते"), Punctuation), QString::fromUtf8("नमस्ते"),
            "leaves Devanagari untouched");
    checkEq(normalizeWord(QString::fromUtf8("(नमस्ते)"), Punctuation), QString::fromUtf8("नमस्ते"),
            "peels brackets around Devanagari");
}

// ---------------------------------------------------------------------- isTimeStamp
static void testIsTimeStamp()
{
    check(isTimeStamp("01:02:03"), "accepts hh:mm:ss");
    check(isTimeStamp("01:02:03.400"), "accepts hh:mm:ss.zzz");
    check(isTimeStamp("23:59:59"), "accepts the last second of the day");
    check(isTimeStamp("00:00:00"), "accepts midnight");

    check(!isTimeStamp("24:00:00"), "rejects a 24th hour");
    check(!isTimeStamp("01:60:00"), "rejects a 60th minute");
    check(!isTimeStamp("hello"), "rejects a word");
    check(!isTimeStamp(""), "rejects empty text");

    // Anchored at both ends, so a word that merely contains a timestamp is still a word.
    check(!isTimeStamp("x01:02:03"), "rejects a timestamp with a prefix");
    check(!isTimeStamp("01:02:03x"), "rejects a timestamp with a suffix");
}

// ----------------------------------------------------------------------------- scan
static block makeBlock(const QString& speaker, const QStringList& words, const QTime& timeStamp,
                       const QStringList& blockTags = {})
{
    QVector<word> wordVector;
    for (const QString& w : words)
        wordVector.append(word(timeStamp, w, {}, "false"));

    return block(timeStamp, words.join(' '), speaker, blockTags, wordVector);
}

static void testScan()
{
    // No dictionary is mapped, so every word is unknown. That isolates the block level
    // rules from dictionary content.
    Dictionary empty;
    Dictionary english;

    QVector<block> blocks;
    blocks.append(makeBlock("A", {"one", "two"}, QTime(0, 0, 1, 0)));
    blocks.append(makeBlock("A", {"three"}, QTime()));                       // no timestamp
    blocks.append(makeBlock("B", {"four"}, QTime(0, 0, 3, 0), {"topic"}));   // tagged block

    const Markers markers = scan(blocks, empty, english, "english", Punctuation);

    check(markers.invalidBlocks == QList<int>{1}, "a block without a timestamp is invalid");
    check(markers.taggedBlocks == QList<int>{2}, "a block with a tag is tagged");

    // Words inside invalid or tagged blocks are not examined, which is the long
    // standing behaviour.
    // uniqueKeys(), not keys(): a QMultiMap returns one key per stored item.
    check(markers.invalidWords.uniqueKeys() == QList<int>{0}, "only words of normal blocks are examined");
    check(markers.invalidWords.values(0).size() == 2, "both unknown words of block 0 are flagged");

    // A word marked as edited is reported regardless of spelling.
    blocks[0].words[0].isEdited = "true";
    const Markers edited = scan(blocks, empty, english, "english", Punctuation);
    check(edited.editedWords.values(0) == QList<int>{0}, "an edited word is reported");

    // A tagged word is reported separately from a tagged block.
    blocks[0].words[1].tagList = QStringList{"name"};
    const Markers tagged = scan(blocks, empty, english, "english", Punctuation);
    check(tagged.taggedWords.values(0) == QList<int>{1}, "a tagged word is reported");

    // An inline timestamp must never be reported as a misspelling.
    QVector<block> withTime;
    withTime.append(makeBlock("A", {"hello", "01:02:03.400"}, QTime(0, 0, 1, 0)));
    const Markers timed = scan(withTime, empty, english, "english", Punctuation);
    check(!timed.invalidWords.values(0).contains(1), "an inline timestamp is not a misspelling");
    check(timed.invalidWords.values(0).contains(0), "a genuinely unknown word is still flagged");

    // Words found in the overlay are accepted, including bracketed ones, which is the
    // inconsistency that unifying the four copies removed.
    Dictionary withWords;
    withWords.addWord("hello");
    QVector<block> bracketed;
    bracketed.append(makeBlock("A", {"(hello)", "\"hello\"", "hello,", "nope"}, QTime(0, 0, 1, 0)));
    const Markers b = scan(bracketed, withWords, english, "english", Punctuation);
    check(!b.invalidWords.values(0).contains(0), "a bracketed known word is not flagged");
    check(!b.invalidWords.values(0).contains(1), "a quoted known word is not flagged");
    check(!b.invalidWords.values(0).contains(2), "a known word with a comma is not flagged");
    check(b.invalidWords.values(0).contains(3), "an unknown word is still flagged");

    // For a non-English transcript the English dictionary is consulted as a fallback.
    Dictionary englishWords;
    englishWords.addWord("computer");
    QVector<block> hindi;
    hindi.append(makeBlock("A", {"computer", "nope"}, QTime(0, 0, 1, 0)));
    const Markers h = scan(hindi, empty, englishWords, "hindi", Punctuation);
    check(!h.invalidWords.values(0).contains(0), "English fallback applies for a Hindi transcript");
    check(h.invalidWords.values(0).contains(1), "the fallback does not accept everything");

    const Markers e = scan(hindi, empty, englishWords, "english", Punctuation);
    check(e.invalidWords.values(0).contains(0), "no fallback when the transcript is English");

    QVector<block> none;
    const Markers emptyMarkers = scan(none, empty, english, "english", Punctuation);
    check(emptyMarkers.invalidBlocks.isEmpty() && emptyMarkers.invalidWords.isEmpty(),
          "an empty transcript produces no markers");
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    testNormalizeWord();
    testIsTimeStamp();
    testScan();

    if (g_failures == 0) {
        qInfo().noquote() << "ALL ANALYSIS CHECKS PASSED";
        return 0;
    }
    qWarning().noquote() << g_failures << "CHECK(S) FAILED";
    return 1;
}
