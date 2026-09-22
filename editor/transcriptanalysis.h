#pragma once

#include "blockandword.h"
#include "dictionary.h"

#include <QList>
#include <QMultiMap>
#include <QString>
#include <QVector>

/*!
 * \brief Classifies a transcript for the highlighter: which blocks and words are
 *        misspelled, tagged or edited.
 *
 * This scan previously existed in four places inside Editor, and they did not agree.
 * Two of them stripped surrounding brackets and quotes before looking a word up and
 * two did not, so whether "(hello)" was underlined as a misspelling depended on
 * whether the last thing to run was a content change or a dictionary reload. Having
 * one implementation removes that, and makes the rules testable without a widget.
 */
namespace TranscriptAnalysis {

/*! \brief Everything the highlighter needs in order to format a transcript. */
struct Markers
{
    QList<int> invalidBlocks;           ///< Blocks with no timestamp.
    QList<int> taggedBlocks;            ///< Blocks carrying a tag.
    QMultiMap<int, int> invalidWords;   ///< Block to word index, for misspellings.
    QMultiMap<int, int> taggedWords;    ///< Block to word index, for tagged words.
    QMultiMap<int, int> editedWords;    ///< Block to word index, for edited words.
};

/*!
 * \brief Strips what should not take part in a dictionary lookup.
 *
 * Lowercases, drops one trailing punctuation mark, then peels paired delimiters
 * (quotes, brackets, braces, angle brackets, apostrophes) and any remaining trailing
 * question mark, exclamation mark or comma.
 *
 * \param punctuation Characters treated as trailing punctuation, usually ",.!;:?".
 */
QString normalizeWord(const QString& text, const QString& punctuation);

/*!
 * \brief True when the text is a bare timestamp such as "01:02:03.400".
 *
 * Timestamps appear as words in the document when timestamps are shown, and must
 * never be reported as misspellings.
 */
bool isTimeStamp(const QString& text);

/*!
 * \brief True when \a word appears in the primary dictionary, or in the English
 *        dictionary when the transcript is not in English.
 */
bool isWordValid(const QString& word, const Dictionary& primaryDict,
                 const Dictionary& englishDict, const QString& language);

/*!
 * \brief Classifies every block and word in \a blocks.
 *
 * A block with no timestamp is invalid and its words are not examined. A block with a
 * tag is marked as tagged and its words are likewise skipped, which is the behaviour
 * the editor has always had.
 */
Markers scan(const QVector<block>& blocks, const Dictionary& primaryDict,
             const Dictionary& englishDict, const QString& language,
             const QString& punctuation);

} // namespace TranscriptAnalysis
