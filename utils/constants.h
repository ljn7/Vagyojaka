#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QRegularExpression>
#include <qstandardpaths.h>

namespace Constants {
namespace Vagyojaka {
extern const QString APPDATA_BASE_DIR;
extern const QString CONFIG_INI;
}
namespace Text {
extern const QRegularExpression WHITESPACE_NORMALIZER;
}
}
#endif // CONSTANTS_H
