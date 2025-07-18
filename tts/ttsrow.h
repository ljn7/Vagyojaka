#ifndef TTSROW_H
#define TTSROW_H
#include "qstring.h"

struct TTSRow {
    QString words;
    QString audioFileName;
    QString not_pronounced_properly;
    QString tag;
    int sound_quality;
    int asr_quality;
    bool wordsEdited = false;
    bool pronunciationEdited = false;
    bool tagEdited = false;
};

#endif // TTSBLOCK_H
