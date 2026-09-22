#include "transcriptgenerator.h"
#include <QFileDialog>
#include<QStandardPaths>
#include<QDir>
#include<QDebug>
#include <QProgressBar>
#include<QApplication>
#include<QWidget>
#include<QMessageBox>
#include "tool.h"
#include "./ui_tool.h"
#include "util/scriptrunner.h"
TranscriptGenerator::TranscriptGenerator(QObject *parent,    QUrl *fileUr)
    : QObject{parent}
{

    fileUrl=fileUr;
    qInfo()<<*fileUrl;
}

TranscriptGenerator::TranscriptGenerator(QUrl *fileUr)
{

    fileUrl=fileUr;
    qInfo()<<*fileUrl;
}

void TranscriptGenerator::Upload_and_generate_Transcript()
{
    QProgressBar progressBar;
            progressBar.setMinimum(0);
            progressBar.setMaximum(100);
            progressBar.setValue(10);
            progressBar.show();
            progressBar.raise();
            progressBar.activateWindow();

    QFile myfile(fileUrl->toLocalFile());
    QFileInfo fileInfo(myfile);
    QString filename(fileInfo.fileName());
    QString filepaths=fileInfo.dir().path();
    QString filepaths2=filepaths;

    QString extractError;
    if (!ScriptRunner::ensureExtracted(":/client.py", "client.py", &extractError)) {
        QMessageBox::critical(nullptr, QObject::tr("Error"), extractError);
        return;
    }

    const QStringList clientArgs{
        fileInfo.absoluteFilePath(),
        filepaths + "/transcript.xml",
    };

    const auto generation = ScriptRunner::runPython("client.py", clientArgs, nullptr,
                                                    QObject::tr("Generating transcript..."),
                                                    600000);
    if (!generation.ok) {
        QMessageBox::critical(nullptr, QObject::tr("Transcript generation failed"), generation.error);
        return;
    }

    // client.py has exited, so the transcript either exists by now or never will.
    // The old code spun here and hung the application forever whenever it failed.
    if (!QFileInfo(filepaths2 + "/transcript.xml").isFile()) {
        QMessageBox::critical(nullptr, QObject::tr("Transcript generation failed"),
                              QObject::tr("client.py finished without producing %1.")
                                  .arg(filepaths2 + "/transcript.xml"));
        return;
    }


            progressBar.setValue(100);
            progressBar.hide();



}
