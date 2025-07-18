#include "constants.h"
#include <qdir.h>

namespace Constants {
namespace Vagyojaka {
extern const QString APPDATA_BASE_DIR = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
extern const QString CONFIG_INI = APPDATA_BASE_DIR + QDir::separator() + "config.ini";
}
namespace Text {
const QRegularExpression WHITESPACE_NORMALIZER("\\s{2,}");
}
}
