#pragma once

#include "blockandword.h"

#include <QString>
#include <QTime>
#include <QVector>

/*!
 * \brief Structural edits on a transcript's block list.
 *
 * These were methods on Editor that mutated the block list and re-rendered the widget
 * in one go, which meant the editing rules could only be exercised by driving a text
 * cursor. They are pure functions over a QVector<block> here, so the rules are
 * testable; Editor keeps the cursor handling and the re-render.
 *
 * Timestamps mark the end of a line, which is why merging keeps the later of the two.
 */
namespace TranscriptEdits {

/*! \brief Why an edit did or did not happen. */
enum class Result
{
    Applied,        ///< The block list was modified.
    NotApplicable,  ///< Preconditions not met, for example merging across two speakers.
    InvalidRange,   ///< A block range fell outside the transcript.
    InvalidTime,    ///< A null or unusable time was supplied.
};

/*!
 * \brief Merges the block at \a blockNumber into the one above it.
 *
 * Only merges when both blocks have the same speaker. The surviving block keeps the
 * later timestamp, and \a blockNumber is removed.
 */
Result mergeUp(QVector<block>& blocks, int blockNumber);

/*!
 * \brief Merges the block at \a blockNumber into the one below it.
 *
 * Only merges when both blocks have the same speaker. The surviving block keeps the
 * later timestamp, and \a blockNumber is removed.
 */
Result mergeDown(QVector<block>& blocks, int blockNumber);

/*!
 * \brief Shifts the timestamps of a range of blocks by \a delta.
 *
 * \param start  First block, 1 based and inclusive.
 * \param end    Last block, 1 based and inclusive.
 * \param negate Subtracts \a delta instead of adding it.
 *
 * Blocks with a null timestamp are treated as starting from 00:00:00.000.
 */
Result propagateTime(QVector<block>& blocks, const QTime& delta, int start, int end, bool negate);

/*!
 * \brief Renames the speaker of one block.
 *
 * \param replaceAllOccurrences Renames every block that shares the original speaker.
 */
Result changeSpeaker(QVector<block>& blocks, int blockNumber, const QString& newSpeaker,
                     bool replaceAllOccurrences);

} // namespace TranscriptEdits
