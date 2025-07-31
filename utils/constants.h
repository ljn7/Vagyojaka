#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QRegularExpression>
#include <qbrush.h>
#include <qcolor.h>
#include <qstandardpaths.h>

namespace Constants {

namespace Vagyojaka {
    extern const QString APPDATA_BASE_DIR;
    extern const QString CONFIG_INI;
}

namespace Text {
    extern const QRegularExpression WHITESPACE_NORMALIZER;
}

namespace Colors {
    extern const QColor Orange;
    extern const QColor Yellow;
    extern const QColor Apple;
    extern const QColor Peppermint;
    extern const QColor Froly;
    extern const QColor Azalea;
}

namespace Brush {
    extern const QBrush Orange;
    extern const QBrush Yellow;
    extern const QBrush Red;
    extern const QBrush Apple;
    extern const QBrush Peppermint;
    extern const QBrush Froly;
    extern const QBrush Azalea;
}

}
#endif // CONSTANTS_H
