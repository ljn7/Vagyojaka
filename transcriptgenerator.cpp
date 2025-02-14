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

    if(!QFile::exists("client.py")){
        QFile mapper("client.py");
        QFileInfo mapperFileInfo(mapper);
        QFile aligner(":/client.py");
        if(!aligner.open(QIODevice::OpenModeFlag::ReadOnly)){
            return;
        }
        aligner.seek(0);
        QString cp=aligner.readAll();
        aligner.close();

        if(!mapper.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::Truncate)){

            return;
        }
        mapper.write(QByteArray(cp.toUtf8()));
        mapper.close();
        std::string makingexec="chmod +x "+mapperFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString();
        int result = system(makingexec.c_str());
        // qInfo()<<result; // Disabled debug
    }
    QFile client_script("client.py");
    QFileInfo client_script_info(client_script);
    std::string client="python3 "
        +client_script_info.absoluteFilePath().replace(" ", "\\ ").toStdString()
        +" "
        +fileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString()
        +" "
        +filepaths.replace(" ", "\\ ").toStdString()
        +"/transcript.xml";
    int result = system(client.c_str());
    qInfo()<<result;

    bool fileExists = QFileInfo::exists(filepaths2+"/transcript.xml") && QFileInfo(filepaths2+"/transcript.xml").isFile();
    while(!fileExists){
        fileExists = QFileInfo::exists(filepaths2+"/transcript.xml") && QFileInfo(filepaths2+"/transcript.xml").isFile();
    }


            progressBar.setValue(100);
            progressBar.hide();



}
