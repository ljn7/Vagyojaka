#pragma once

#include <QStringList>
#include <QTime>
#include <QVector>

#include <utility>

/*!
 * \brief One word of a transcript, with the time it starts at.
 *
 * Comparison deliberately ignores \c tagList: two words are the same word whether or
 * not a tag has been attached to them. That was the behaviour of the original
 * hand-written operator== and the diffing in Editor::contentChanged relies on it.
 */
struct word
{
    QTime timeStamp;
    QString text;
    QStringList tagList;
    QString isEdited{QStringLiteral("false")};

    word() = default;

    word(QTime timeStamp, QString text, QStringList tagList,
         QString isEdited = QStringLiteral("false"))
        : timeStamp(timeStamp)
        , text(std::move(text))
        , tagList(std::move(tagList))
        , isEdited(std::move(isEdited))
    {
    }

    // Taken by reference rather than by value. These hold a QString and a QStringList,
    // so the old by-value signature deep copied both on every comparison, and the word
    // diff in contentChanged compares in nested loops.
    bool operator==(const word& other) const
    {
        return timeStamp == other.timeStamp && text == other.text && isEdited == other.isEdited;
    }
};

/*!
 * \brief One line of a transcript: a speaker, a timestamp and the words that follow.
 *
 * As with \c word, comparison ignores \c tagList.
 */
struct block
{
    QTime timeStamp;
    QString text;
    QString speaker;
    QStringList tagList;
    QVector<word> words;

    block() = default;

    block(QTime timeStamp, QString text, QString speaker, QStringList tagList, QVector<word> words)
        : timeStamp(timeStamp)
        , text(std::move(text))
        , speaker(std::move(speaker))
        , tagList(std::move(tagList))
        , words(std::move(words))
    {
    }

    bool operator==(const block& other) const
    {
        return timeStamp == other.timeStamp && text == other.text && speaker == other.speaker
               && words == other.words;
    }
};

Q_DECLARE_METATYPE(block)
