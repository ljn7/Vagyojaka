#include "transcriptexporter.h"

#include <QFile>
#include <QPageSize>
#include <QPrinter>
#include <QTextDocument>
#include <QTextStream>

#include <utility>

namespace {

constexpr auto TimeStampFormat = "hh:mm:ss.zzz";

constexpr auto HtmlPrologue =
    "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
    "<meta http-equiv='X-UA-Compatible' content='IE=edge'>"
    "<meta name='viewport' content='width= 290 , initial-scale=1.0'>"
    "<title>Document</title></head><body>\n";

constexpr auto HtmlEpilogue = "</body></html>";

} // namespace

namespace TranscriptExporter {

QString toHtml(const QVector<block>& blocks, bool includeTimeStamps)
{
    QString html(HtmlPrologue);

    for (const block& a_block : std::as_const(blocks)) {
        html += "<p>{" + a_block.speaker + "}: " + a_block.text;
        if (includeTimeStamps)
            html += " {" + a_block.timeStamp.toString(TimeStampFormat) + "}";
        html += "<p>\n\n";
    }

    html += HtmlEpilogue;
    return html;
}

QString toPlainText(const QVector<block>& blocks)
{
    QString text;

    for (const block& a_block : std::as_const(blocks)) {
        text += "{" + a_block.speaker + "}: " + a_block.text + " {"
                + a_block.timeStamp.toString(TimeStampFormat) + "}\n\n";
    }

    return text;
}

bool writePdf(const QString& path, const QString& html, QString* errorOut)
{
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize::A4);
    printer.setOutputFileName(path);

    QTextDocument document;
    document.setHtml(html);
    document.print(&printer);

    if (!QFile::exists(path)) {
        if (errorOut)
            *errorOut = QObject::tr("Could not write %1").arg(path);
        return false;
    }

    return true;
}

bool writeText(const QString& path, const QString& text, QString* errorOut)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut)
            *errorOut = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << text;
    file.close();

    return true;
}

} // namespace TranscriptExporter
