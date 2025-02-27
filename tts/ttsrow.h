#ifndef TTSROW_H
#define TTSROW_H
#include "qstring.h"

enum Verification {
    not_verified = 0,
    correct = 1,
    wrong = 2
};

struct TTSRow {
    QString words;
    QString audioFileName;
    QString not_pronounced_properly;
    QString tag;
    int sound_quality = 0;
    int asr_quality = 0;
    Verification verification = Verification::not_verified;
};

#endif // TTSBLOCK_H
