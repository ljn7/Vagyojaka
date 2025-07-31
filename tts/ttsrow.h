#ifndef TTSROW_H
#define TTSROW_H
#include "qstring.h"

struct TTSRow {
    QString words;
    QString audioFileName;
    QString comments;
    QString tags;
    QString wer;
    QString hypothesis;

    bool wordsEdited = false;
    bool commentsEdited = false;
    bool tagsEdited = false;
    bool markAsHighWER = false;
};

#endif // TTSBLOCK_H
