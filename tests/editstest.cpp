// Checks for the structural transcript edits.
//
// These operations rewrite the user's lines in place, and a mistake in one of them is
// the kind of thing that is only noticed after the file has been saved.
#include "editor/transcriptedits.h"

#include <QCoreApplication>
#include <QDebug>

using TranscriptEdits::Result;

static int g_failures = 0;

static void check(bool ok, const QString& what)
{
    if (!ok) {
        qWarning().noquote() << "FAIL:" << what;
        ++g_failures;
    }
}

static block makeBlock(const QString& speaker, const QString& text, const QTime& timeStamp)
{
    QVector<word> words;
    const QStringList parts = text.split(' ', Qt::SkipEmptyParts);
    for (const QString& part : parts)
        words.append(word(timeStamp, part, {}, "false"));

    return block(timeStamp, text, speaker, {}, words);
}

static QVector<block> sample()
{
    return {
        makeBlock("A", "one two", QTime(0, 0, 5, 0)),
        makeBlock("A", "three four", QTime(0, 0, 10, 0)),
        makeBlock("B", "five six", QTime(0, 0, 15, 0)),
    };
}

// ------------------------------------------------------------------------- mergeUp
static void testMergeUp()
{
    QVector<block> blocks = sample();

    check(TranscriptEdits::mergeUp(blocks, 1) == Result::Applied, "mergeUp same speaker applies");
    check(blocks.size() == 2, "mergeUp removes a block");
    check(blocks[0].text == "one two three four", QStringLiteral("mergeUp joins text, got: %1").arg(blocks[0].text));
    check(blocks[0].words.size() == 4, "mergeUp joins words");
    // Timestamps mark the end of a line, so the merged line ends where the later did.
    check(blocks[0].timeStamp == QTime(0, 0, 10, 0), "mergeUp keeps the later timestamp");
    check(blocks[0].speaker == "A", "mergeUp keeps the speaker");

    // Different speakers must not merge.
    blocks = sample();
    check(TranscriptEdits::mergeUp(blocks, 2) == Result::NotApplicable, "mergeUp across speakers is refused");
    check(blocks.size() == 3, "refused mergeUp leaves the list alone");

    // The first block has nothing above it.
    blocks = sample();
    check(TranscriptEdits::mergeUp(blocks, 0) == Result::NotApplicable, "mergeUp on the first block is refused");
    check(blocks.size() == 3, "refused mergeUp on first block leaves the list alone");

    // Out of range and empty input must not read past the end.
    blocks = sample();
    check(TranscriptEdits::mergeUp(blocks, 99) == Result::NotApplicable, "mergeUp past the end is refused");
    check(TranscriptEdits::mergeUp(blocks, -1) == Result::NotApplicable, "mergeUp with a negative index is refused");

    QVector<block> empty;
    check(TranscriptEdits::mergeUp(empty, 0) == Result::NotApplicable, "mergeUp on an empty transcript is refused");
}

// ----------------------------------------------------------------------- mergeDown
static void testMergeDown()
{
    QVector<block> blocks = sample();

    check(TranscriptEdits::mergeDown(blocks, 0) == Result::Applied, "mergeDown same speaker applies");
    check(blocks.size() == 2, "mergeDown removes a block");
    // The earlier line's words lead, which is the ordering the old implementation
    // achieved with a temporary swap.
    check(blocks[0].text == "one two three four", QStringLiteral("mergeDown joins text in order, got: %1").arg(blocks[0].text));
    check(blocks[0].words.size() == 4, "mergeDown joins words");
    check(blocks[0].words.first().text == "one", "mergeDown puts the earlier line first");
    check(blocks[0].words.last().text == "four", "mergeDown puts the later line last");
    check(blocks[0].timeStamp == QTime(0, 0, 10, 0), "mergeDown keeps the later timestamp");

    blocks = sample();
    check(TranscriptEdits::mergeDown(blocks, 1) == Result::NotApplicable, "mergeDown across speakers is refused");
    check(blocks.size() == 3, "refused mergeDown leaves the list alone");

    blocks = sample();
    check(TranscriptEdits::mergeDown(blocks, 2) == Result::NotApplicable, "mergeDown on the last block is refused");

    blocks = sample();
    check(TranscriptEdits::mergeDown(blocks, 99) == Result::NotApplicable, "mergeDown past the end is refused");
    check(TranscriptEdits::mergeDown(blocks, -1) == Result::NotApplicable, "mergeDown with a negative index is refused");

    QVector<block> empty;
    check(TranscriptEdits::mergeDown(empty, 0) == Result::NotApplicable, "mergeDown on an empty transcript is refused");
}

// Merging up at N and merging down at N-1 must produce the same transcript.
static void testMergeSymmetry()
{
    QVector<block> viaUp = sample();
    QVector<block> viaDown = sample();

    check(TranscriptEdits::mergeUp(viaUp, 1) == Result::Applied, "symmetry: mergeUp applied");
    check(TranscriptEdits::mergeDown(viaDown, 0) == Result::Applied, "symmetry: mergeDown applied");

    check(viaUp.size() == viaDown.size(), "symmetry: same block count");
    if (viaUp.size() == viaDown.size()) {
        for (qsizetype i = 0; i < viaUp.size(); ++i)
            check(viaUp[i] == viaDown[i] && viaUp[i].text == viaDown[i].text,
                  QStringLiteral("symmetry: block %1 matches").arg(i));
    }
}

// ------------------------------------------------------------------- propagateTime
static void testPropagateTime()
{
    QVector<block> blocks = sample();

    check(TranscriptEdits::propagateTime(blocks, QTime(0, 0, 2, 500), 1, 3, false) == Result::Applied,
          "propagateTime forward applies");
    check(blocks[0].timeStamp == QTime(0, 0, 7, 500), "propagateTime adds to block 1");
    check(blocks[1].timeStamp == QTime(0, 0, 12, 500), "propagateTime adds to block 2");
    check(blocks[2].timeStamp == QTime(0, 0, 17, 500), "propagateTime adds to block 3");

    // Negating must return the transcript to where it started.
    check(TranscriptEdits::propagateTime(blocks, QTime(0, 0, 2, 500), 1, 3, true) == Result::Applied,
          "propagateTime backward applies");
    check(blocks[0].timeStamp == QTime(0, 0, 5, 0), "propagateTime negation is the inverse");
    check(blocks[2].timeStamp == QTime(0, 0, 15, 0), "propagateTime negation is the inverse for the last block");

    // A partial range must leave blocks outside it untouched.
    blocks = sample();
    check(TranscriptEdits::propagateTime(blocks, QTime(0, 0, 1, 0), 2, 2, false) == Result::Applied,
          "propagateTime on a single block applies");
    check(blocks[0].timeStamp == QTime(0, 0, 5, 0), "propagateTime leaves earlier blocks alone");
    check(blocks[1].timeStamp == QTime(0, 0, 11, 0), "propagateTime shifts only the chosen block");
    check(blocks[2].timeStamp == QTime(0, 0, 15, 0), "propagateTime leaves later blocks alone");

    // Hours, minutes and milliseconds all contribute.
    blocks = sample();
    TranscriptEdits::propagateTime(blocks, QTime(1, 2, 3, 4), 1, 1, false);
    check(blocks[0].timeStamp == QTime(1, 2, 8, 4), "propagateTime handles hours, minutes and milliseconds");

    // A null timestamp is treated as midnight rather than staying null.
    blocks = sample();
    blocks[0].timeStamp = QTime();
    TranscriptEdits::propagateTime(blocks, QTime(0, 0, 3, 0), 1, 1, false);
    check(blocks[0].timeStamp == QTime(0, 0, 3, 0), "propagateTime initialises a null timestamp");

    // Ranges are 1 based and inclusive; anything outside is rejected without mutating.
    blocks = sample();
    check(TranscriptEdits::propagateTime(blocks, QTime(0, 0, 1, 0), 0, 3, false) == Result::InvalidRange,
          "propagateTime rejects a zero start");
    check(TranscriptEdits::propagateTime(blocks, QTime(0, 0, 1, 0), 1, 4, false) == Result::InvalidRange,
          "propagateTime rejects an end past the last block");
    check(TranscriptEdits::propagateTime(blocks, QTime(0, 0, 1, 0), 3, 1, false) == Result::InvalidRange,
          "propagateTime rejects an inverted range");
    check(blocks[0].timeStamp == QTime(0, 0, 5, 0), "rejected propagateTime leaves timestamps alone");

    check(TranscriptEdits::propagateTime(blocks, QTime(), 1, 3, false) == Result::InvalidTime,
          "propagateTime rejects a null time");
}

// -------------------------------------------------------------------- changeSpeaker
static void testChangeSpeaker()
{
    QVector<block> blocks = sample();

    check(TranscriptEdits::changeSpeaker(blocks, 0, "Z", false) == Result::Applied, "changeSpeaker single applies");
    check(blocks[0].speaker == "Z", "changeSpeaker renames the chosen block");
    check(blocks[1].speaker == "A", "changeSpeaker single leaves other blocks alone");

    blocks = sample();
    check(TranscriptEdits::changeSpeaker(blocks, 0, "Z", true) == Result::Applied, "changeSpeaker all applies");
    check(blocks[0].speaker == "Z", "changeSpeaker all renames the chosen block");
    check(blocks[1].speaker == "Z", "changeSpeaker all renames the other block with that speaker");
    check(blocks[2].speaker == "B", "changeSpeaker all leaves a different speaker alone");

    blocks = sample();
    check(TranscriptEdits::changeSpeaker(blocks, 99, "Z", false) == Result::NotApplicable,
          "changeSpeaker past the end is refused");
    check(TranscriptEdits::changeSpeaker(blocks, -1, "Z", false) == Result::NotApplicable,
          "changeSpeaker with a negative index is refused");

    QVector<block> empty;
    check(TranscriptEdits::changeSpeaker(empty, 0, "Z", false) == Result::NotApplicable,
          "changeSpeaker on an empty transcript is refused");
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    testMergeUp();
    testMergeDown();
    testMergeSymmetry();
    testPropagateTime();
    testChangeSpeaker();

    if (g_failures == 0) {
        qInfo().noquote() << "ALL TRANSCRIPT EDIT CHECKS PASSED";
        return 0;
    }
    qWarning().noquote() << g_failures << "CHECK(S) FAILED";
    return 1;
}
