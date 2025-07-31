#include "constants.h"
#include "qdir"

namespace Constants {

namespace Vagyojaka {
    const QString APPDATA_BASE_DIR = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString CONFIG_INI = APPDATA_BASE_DIR + QDir::separator() + "config.ini";
}

namespace Text {
    const QRegularExpression WHITESPACE_NORMALIZER("\\s{2,}");
}

namespace Colors {
    const QColor Orange = QColor(255, 165, 0);
    const QColor Yellow = QColor(Qt::yellow);
    const QColor Apple = QColor(79, 173, 66);
    const QColor Peppermint = QColor(139, 206, 129);
    const QColor Froly = QColor(245, 129, 129);
    const QColor Azalea = QColor(249, 202, 202);
}

namespace Brush {
    const QBrush Orange(Colors::Orange);
    const QBrush Yellow(Colors::Yellow);
    const QBrush Red(Qt::red);
    const QBrush Apple(Colors::Apple);
    const QBrush Peppermint(Colors::Peppermint);
    const QBrush Froly(Colors::Froly);
    const QBrush Azalea(Colors::Azalea);
}

}
