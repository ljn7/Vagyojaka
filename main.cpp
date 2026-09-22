#include "tool.h"
#include "util/logger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Vagyojaka");
    // app.setApplicationDisplayName("Vagyojaka: ASR Post Editor");
    app.setOrganizationName("IIT Bombay");
    app.setApplicationVersion(APP_VERSION);

    // Installed after the application object exists, because the log directory comes
    // from QStandardPaths and that depends on the organisation and application names.
    Logger::install();

    Tool w;
    w.show();

    return app.exec();
}
