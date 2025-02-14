#pragma once

#include <QVector>
#include <QTime>

struct word
{
    QTime timeStamp;
    QString text;
    QStringList tagList;
    QString isEdited;
    QString isEditedInternal;

    word(QTime timeStamp, QString text, QStringList tagList, QString isEdited = "false", QString isEditedInternal = "false")
        : timeStamp(timeStamp), text(text), tagList(tagList), isEdited(isEdited), isEditedInternal(isEditedInternal) {}

    word() : timeStamp(), text(), tagList(), isEdited("false"), isEditedInternal("false") {}

    inline bool operator==(word w) const
    {
        return (w.timeStamp == timeStamp && w.text == text && w.isEdited == isEdited && w.isEditedInternal == isEditedInternal);
    }
};

struct block
{
    QTime timeStamp;
    QString text;
    QString speaker;
    QStringList tagList;
    QVector<word> words;

    block() : timeStamp(), text(), speaker(), tagList(), words() {};

    block(QTime timeStamp, QString text, QString speaker, QStringList tagList, QVector<word> words)
        : timeStamp(timeStamp), text(text), speaker(speaker), tagList(tagList), words(words) {};

    inline bool operator==(block b) const
    {
        if(b.timeStamp==timeStamp && b.text==text && b.speaker==speaker && b.words==words)
            return true;
        return false;
    }
};

Q_DECLARE_METATYPE(block)
