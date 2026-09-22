#include "transcriptanalysis.h"

#include <QRegularExpression>

namespace {

struct Delimiter
{
    char open;
    char close;
};

/*! Peeled in pairs, in this order. */
constexpr Delimiter Delimiters[] = {
    {'"', '"'}, {'(', ')'}, {'[', ']'}, {'{', '}'}, {'\'', '\''}, {'<', '>'},
};

/*! Trailing marks stripped after the delimiters have been peeled. */
constexpr char TrailingMarks[] = {'?', '!', ','};

} // namespace

namespace TranscriptAnalysis {

QString normalizeWord(const QString& text, const QString& punctuation)
{
    QString word = text.toLower();

    if (!word.isEmpty() && punctuation.contains(word.back()))
        word.chop(1);

    for (const auto& delimiter : Delimiters) {
        if (!word.isEmpty() && word.front() == QLatin1Char(delimiter.open))
            word.remove(0, 1);
        if (!word.isEmpty() && word.back() == QLatin1Char(delimiter.close))
            word.chop(1);
    }

    for (char mark : TrailingMarks) {
        if (!word.isEmpty() && word.back() == QLatin1Char(mark))
            word.chop(1);
    }

    return word;
}

bool isTimeStamp(const QString& text)
{
    static const QRegularExpression pattern(
        "^([0-1][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(\\.[0-9]+)?$");
    return pattern.match(text).hasMatch();
}

bool isWordValid(const QString& word, const Dictionary& primaryDict,
                 const Dictionary& englishDict, const QString& language)
{
    if (primaryDict.contains(word))
        return true;

    if (language != QStringLiteral("english"))
        return englishDict.contains(word);

    return false;
}

Markers scan(const QVector<block>& blocks, const Dictionary& primaryDict,
             const Dictionary& englishDict, const QString& language,
             const QString& punctuation)
{
    Markers markers;

    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].timeStamp.isNull()) {
            markers.invalidBlocks.append(i);
            continue;
        }

        if (!blocks[i].tagList.isEmpty()) {
            markers.taggedBlocks.append(i);
            continue;
        }

        for (int j = 0; j < blocks[i].words.size(); j++) {
            const word& a_word = blocks[i].words[j];

            if (a_word.isEdited == QStringLiteral("true"))
                markers.editedWords.insert(i, j);

            if (!a_word.tagList.isEmpty())
                markers.taggedWords.insert(i, j);

            const QString normalized = normalizeWord(a_word.text, punctuation);

            // A timestamp shown inline is not a word and is never a misspelling.
            if (isTimeStamp(normalized))
                continue;

            if (!isWordValid(normalized, primaryDict, englishDict, language))
                markers.invalidWords.insert(i, j);
        }
    }

    return markers;
}

} // namespace TranscriptAnalysis
