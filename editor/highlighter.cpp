#include "highlighter.h"

#include <QColor>
#include <QFont>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextBlock>
#include <QTextCharFormat>

#include <utility>

namespace {

/*! \brief The speaker prefix pattern, compiled once rather than on every block. */
const QRegularExpression& speakerRegex()
{
    static const QRegularExpression re(R"(\{.*\}:)");
    return re;
}

/*! \brief The timestamp pattern, compiled once rather than on every block. */
const QRegularExpression& timeStampRegex()
{
    static const QRegularExpression re(R"(\{(\d?\d:)?[0-5]?\d:[0-5]?\d(\.\d\d?\d?)?\})");
    return re;
}

/*!
 * \brief Splits a block into words and records where each one starts in the block text.
 *
 * Highlighting used to recompute a word offset by summing the lengths of every word
 * before it, inside the loop, for every format it applied. That made a block quadratic
 * in its word count, and it was repeated up to seven times per block. The offsets are
 * now accumulated once and indexed.
 */
struct BlockWords
{
    int speakerEnd = 0;
    QStringList words;
    QList<int> starts; ///< Index into the block text where each word begins.

    explicit BlockWords(const QString& text)
    {
        const auto speakerMatch = speakerRegex().match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        words = text.mid(speakerEnd + 1).split(" ");
        starts.reserve(words.size());

        int offset = speakerEnd + 1;
        for (const QString& word : std::as_const(words)) {
            starts.append(offset);
            offset += word.size() + 1;
        }
    }

    qsizetype size() const { return words.size(); }
};

/*! \brief Turns the multimap lookup into a set so membership tests are not linear. */
QSet<int> wordNumbersFor(const QMultiMap<int, int>& map, int blockNumber)
{
    const QList<int> values = map.values(blockNumber);
    return QSet<int>(values.cbegin(), values.cend());
}

} // namespace

void Highlighter::highlightBlock(const QString& text)
{
    const int blockNumber = currentBlock().blockNumber();

    if (invalidBlockNumbers.contains(blockNumber)) {
        QTextCharFormat format;
        format.setForeground(Qt::red);
        setFormat(0, text.size(), format);
        return;
    }
    else if (taggedBlockNumbers.contains(blockNumber)) {
        QTextCharFormat format;
        format.setForeground(Qt::blue);
        setFormat(0, text.size(), format);
        return;
    }

    if (invalidWords.contains(blockNumber)) {
        const QSet<int> invalidWordNumbers = wordNumbersFor(invalidWords, blockNumber);
        const BlockWords blockWords(text);

        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineColor(Qt::red);
        format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        for (int i = 0; i < blockWords.size(); i++) {
            if (!invalidWordNumbers.contains(i))
                continue;
            setFormat(blockWords.starts[i], blockWords.words[i].size(), format);
        }
    }

    if (taggedWords.contains(blockNumber)) {
        const QSet<int> taggedWordNumbers = wordNumbersFor(taggedWords, blockNumber);
        const BlockWords blockWords(text);

        QTextCharFormat format;
        format.setForeground(Qt::blue);

        for (int i = 0; i < blockWords.size(); i++) {
            if (!taggedWordNumbers.contains(i))
                continue;
            setFormat(blockWords.starts[i], blockWords.words[i].size(), format);
        }
    }

    if (!editedWords.isEmpty()) {
        const QSet<int> editedWordNumbers = wordNumbersFor(editedWords, blockNumber);
        const BlockWords blockWords(text);

        QTextCharFormat format;
        format.setBackground(Qt::yellow);

        for (int i = 0; i < blockWords.size(); i++) {
            if (!editedWordNumbers.contains(i))
                continue;
            setFormat(blockWords.starts[i], blockWords.words[i].size(), format);
        }
    }

    if (blockToHighlight == -1)
        return;

    if (blockNumber != blockToHighlight)
        return;

    const BlockWords blockWords(text);
    const int speakerEnd = blockWords.speakerEnd;
    const int lineEnd = text.length();
    const int timeStampStart = timeStampRegex().match(text).capturedStart();

    QTextCharFormat format;

    format.setForeground(QColor(Qt::blue).lighter(120));
    setFormat(0, speakerEnd, format);
    format.setFontWeight(QFont::Bold);
    setFormat(0, speakerEnd, format);

    format.setForeground(Qt::black);
    setFormat(speakerEnd, lineEnd, format);
    format.setFontWeight(QFont::Bold);
    setFormat(speakerEnd, lineEnd, format);

    if (taggedWords.contains(blockNumber) || invalidWords.contains(blockNumber)) {
        const QSet<int> taggedWordNumbers = wordNumbersFor(taggedWords, blockNumber);
        const QSet<int> invalidWordNumbers = wordNumbersFor(invalidWords, blockNumber);

        QTextCharFormat invalidFormat;
        invalidFormat.setForeground(Qt::black);
        invalidFormat.setFontWeight(QFont::Bold);
        invalidFormat.setFontUnderline(true);
        invalidFormat.setUnderlineColor(Qt::red);
        invalidFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        QTextCharFormat taggedFormat;
        taggedFormat.setFontWeight(QFont::Bold);
        taggedFormat.setForeground(Qt::blue);

        QTextCharFormat bothFormat;
        bothFormat.setFontWeight(QFont::Bold);
        bothFormat.setForeground(Qt::blue);
        bothFormat.setFontUnderline(true);
        bothFormat.setUnderlineColor(Qt::red);
        bothFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        for (int i = 0; i < blockWords.size(); i++) {
            const bool isInvalid = invalidWordNumbers.contains(i);
            const bool isTagged = taggedWordNumbers.contains(i);
            if (!isInvalid && !isTagged)
                continue;

            const int start = blockWords.starts[i];
            const int count = blockWords.words[i].size();

            if (isInvalid)
                setFormat(start, count, invalidFormat);
            if (isTagged)
                setFormat(start, count, taggedFormat);
            if (isInvalid && isTagged)
                setFormat(start, count, bothFormat);
        }
    }

    // capturedStart() returns -1 when the block carries no timestamp.
    if (timeStampStart >= 0) {
        format.setForeground(Qt::red);
        setFormat(timeStampStart, text.size(), format);
        format.setFontWeight(QFont::Light);
        setFormat(timeStampStart, text.size(), format);
    }

    if (wordToHighlight != -1 && wordToHighlight < blockWords.size()) {
        format.setFontUnderline(true);
        format.setUnderlineColor(Qt::green);
        format.setUnderlineStyle(QTextCharFormat::DashUnderline);
        format.setForeground(Qt::green);
        setFormat(blockWords.starts[wordToHighlight], blockWords.words[wordToHighlight].size(), format);
    }
}
