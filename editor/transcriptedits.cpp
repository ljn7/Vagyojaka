#include "transcriptedits.h"

namespace TranscriptEdits {

Result mergeUp(QVector<block>& blocks, int blockNumber)
{
    const int previous = blockNumber - 1;

    if (blocks.isEmpty() || blockNumber <= 0 || blockNumber >= blocks.size())
        return Result::NotApplicable;

    if (blocks[blockNumber].speaker != blocks[previous].speaker)
        return Result::NotApplicable;

    blocks[previous].words.append(blocks[blockNumber].words);
    // The timestamp marks the end of the line, so the merged block ends where the
    // later of the two did.
    blocks[previous].timeStamp = blocks[blockNumber].timeStamp;
    blocks[previous].text.append(" " + blocks[blockNumber].text);

    blocks.removeAt(blockNumber);
    return Result::Applied;
}

Result mergeDown(QVector<block>& blocks, int blockNumber)
{
    const int next = blockNumber + 1;

    if (blocks.isEmpty() || blockNumber < 0 || blockNumber >= blocks.size() - 1)
        return Result::NotApplicable;

    if (blocks[blockNumber].speaker != blocks[next].speaker)
        return Result::NotApplicable;

    // The current block's words come first, because it is the earlier line.
    QVector<word> merged = blocks[blockNumber].words;
    merged.append(blocks[next].words);
    blocks[next].words = merged;

    blocks[next].text = blocks[blockNumber].text + " " + blocks[next].text;

    blocks.removeAt(blockNumber);
    return Result::Applied;
}

Result propagateTime(QVector<block>& blocks, const QTime& delta, int start, int end, bool negate)
{
    if (delta.isNull())
        return Result::InvalidTime;

    if (start < 1 || end > blocks.size() || start > end)
        return Result::InvalidRange;

    int secondsToAdd = delta.hour() * 3600 + delta.minute() * 60 + delta.second();
    int msecondsToAdd = delta.msec();

    if (negate) {
        secondsToAdd = -secondsToAdd;
        msecondsToAdd = -msecondsToAdd;
    }

    for (int i = start - 1; i < end; i++) {
        QTime& timeStamp = blocks[i].timeStamp;

        if (timeStamp.isNull())
            timeStamp = QTime(0, 0, 0, 0);

        timeStamp = timeStamp.addMSecs(msecondsToAdd);
        timeStamp = timeStamp.addSecs(secondsToAdd);
    }

    return Result::Applied;
}

Result changeSpeaker(QVector<block>& blocks, int blockNumber, const QString& newSpeaker,
                     bool replaceAllOccurrences)
{
    if (blocks.isEmpty() || blockNumber < 0 || blockNumber >= blocks.size())
        return Result::NotApplicable;

    const QString previousSpeaker = blocks[blockNumber].speaker;

    if (!replaceAllOccurrences) {
        blocks[blockNumber].speaker = newSpeaker;
        return Result::Applied;
    }

    for (block& a_block : blocks) {
        if (a_block.speaker == previousSpeaker)
            a_block.speaker = newSpeaker;
    }

    return Result::Applied;
}

} // namespace TranscriptEdits
