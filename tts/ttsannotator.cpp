#include "ttsannotator.h"
#include "ui_ttsannotator.h"
#include "lazyloadingmodel.h"
#include "customdelegates.h"
#include "utils/constants.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

const QColor TTSAnnotator::SoundQualityColor = QColor(230, 255, 230);
const QColor TTSAnnotator::TTSQualityColor = QColor(255, 230, 230);

TTSAnnotator::TTSAnnotator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TTSAnnotator)
    , m_model(std::make_unique<LazyLoadingModel>())
{
    ui->setupUi(this);
    tableView = ui->tableView;
    tableView->setModel(m_model.get());
    setDefaultFontOnTableView();
    setupUI();

    QString iniPath = QApplication::applicationDirPath() + "/" + "config.ini";
    settings = std::make_unique<QSettings>(iniPath, QSettings::IniFormat);

    this->supportedFormats = {
        "xml Files (*.xml)",
        "All Files (*)"
    };

    connect(m_model.get(), &LazyLoadingModel::transcriptEditedWordsCount, this, &TTSAnnotator::updateTranscriptEditedWords);
    connect(m_model.get(), &LazyLoadingModel::mispronouncedEditedWordsCount, this, &TTSAnnotator::updateMispronunciationEditedWords);
    connect(m_model.get(), &LazyLoadingModel::taggedEditedWordsCount, this, &TTSAnnotator::updateTagEditedWords);

}

TTSAnnotator::~TTSAnnotator() = default;

void TTSAnnotator::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (const QModelIndex &index : deselected.indexes()) {
        if (index.column() == 0) { // Assuming audio player is in the first column
            tableView->closePersistentEditor(index);
        }
    }
    for (const QModelIndex &index : selected.indexes()) {
        if (index.column() == 0) { // Assuming audio player is in the first column
            tableView->openPersistentEditor(index);
        }
    }
}

void TTSAnnotator::setupUI()
{
    // Set headers for the model
    m_model->setHorizontalHeaderLabels({
        "Audios", "Transcript", "Mispronounced words", "Tags", "Sound Quality", "ASR Quality"
    });

    // Set up delegates
    if (!m_audioPlayerDelegate) {
        m_audioPlayerDelegate = new AudioPlayerDelegate(xmlDirectory, this);
    }
    tableView->setItemDelegateForColumn(0, m_audioPlayerDelegate);
    ComboBoxDelegate* soundQualityDelegate = new ComboBoxDelegate(1, 5, SoundQualityColor.darker(105), this);
    CheckBoxDelegate* ttsQualityDelegate = new CheckBoxDelegate(false, TTSQualityColor.darker(105), this);

    // Add TextEditDelegate for text columns
    textDelegate = new TextEditDelegate(font(), this);
    tableView->setItemDelegateForColumn(1, textDelegate); // Transcript column
    tableView->setItemDelegateForColumn(2, textDelegate); // Mispronounced words column
    tableView->setItemDelegateForColumn(3, textDelegate); // Tags column

    // soundQualityDelegate->
    tableView->setItemDelegateForColumn(4, soundQualityDelegate);
    tableView->setItemDelegateForColumn(5, ttsQualityDelegate);

    // Set up table view properties
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked |
                               QAbstractItemView::EditKeyPressed |
                               QAbstractItemView::AnyKeyPressed);

    // Set up header properties
    // tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // tableView->horizontalHeader()->viewport()->update();

    // Store the column widths after stretch
    // QVector<int> columnWidths;
    // for (int i = 0; i < tableView->model()->columnCount(); ++i) {
    //     columnWidths.append(tableView->columnWidth(i));
    // }
    // tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    // // Restore the stretched widths for columns
    // for (int i = 0; i < columnWidths.size(); ++i) {
    //     tableView->setColumnWidth(i, columnWidths[i]);
    // }
    // tableView->horizontalHeader()->updateGeometry();
    // tableView->horizontalHeader()->viewport()->update();

    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    tableView->setStyleSheet(
        "QTableView::item:selected { background-color: rgba(0, 120, 215, 100); }"
        "QTableView::item:focus { background-color: rgba(0, 120, 215, 50); }"
        );

    // // Enable sorting
    // tableView->setSortingEnabled(true);

    // Set up connections
    connect(tableView, &QTableView::clicked, this, &TTSAnnotator::onCellClicked);
    // connect(tableView->horizontalHeader(), &QHeaderView::sectionResized,
    //         this, &TTSAnnotator::onHeaderResized);

    // Set up button connections
    connect(ui->InsertRowButton, &QPushButton::clicked, this, &TTSAnnotator::insertRow);
    connect(ui->deleteRowButton, &QPushButton::clicked, this, &TTSAnnotator::deleteRow);
    // connect(ui->saveAsTableButton, &QPushButton::clicked, this, &TTSAnnotator::saveAs);
    // connect(ui->saveTableButton, &QPushButton::clicked, this, &TTSAnnotator::save);

    connect(textDelegate, &TextEditDelegate::transcriptWordsEdited,
            this, &TTSAnnotator::updateTranscriptEditedWords);
    connect(textDelegate, &TextEditDelegate::mispronunciationWordsEdited,
            this, &TTSAnnotator::updateMispronunciationEditedWords);
    connect(textDelegate, &TextEditDelegate::tagWordsEdited,
            this, &TTSAnnotator::updateTagEditedWords);

    // Set initial focus
    tableView->setFocus();

    // Resize rows and columns to content
    tableView->resizeRowsToContents();
    // tableView->resizeColumnsToContents();
    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TTSAnnotator::onItemSelectionChanged);

}

void TTSAnnotator::onHeaderResized(int logicalIndex, int oldSize, int newSize)
{
    // Once the user resizes the column, switch to interactive mode
    if (tableView->horizontalHeader()->sectionResizeMode(logicalIndex) != QHeaderView::Interactive) {
        tableView->horizontalHeader()->setSectionResizeMode(logicalIndex, QHeaderView::Interactive);
    }
}

void TTSAnnotator::onItemSelectionChanged()
{
    tableView->viewport()->update();
}

void TTSAnnotator::openTTSTranscript()
{

    if (m_audioPlayerDelegate) {
        static_cast<AudioPlayerDelegate*>(m_audioPlayerDelegate)->clearAllEditors();
    }

    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open File"));
    fileDialog.setNameFilters(supportedFormats);

    if(settings->value("annotatorTranscriptDir").toString().isEmpty())
        fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));
    else
        fileDialog.setDirectory(settings->value("annotatorTranscriptDir").toString());

    if (fileDialog.exec() == QDialog::Accepted) {
        m_model->clear();

        fileUrl = fileDialog.selectedUrls().constFirst();
        xmlDirectory = QFileInfo(fileUrl.toLocalFile()).absolutePath();
        if (m_audioPlayerDelegate)
            m_audioPlayerDelegate->setBaseDir(xmlDirectory);
        parseXML();

        emit openMessage(fileUrl.fileName());
        QFileInfo filedir(fileUrl.toLocalFile());
        QString dirInString = filedir.dir().path();
        settings->setValue("annotatorTranscriptDir", dirInString);
    }
}

void TTSAnnotator::parseXML()
{
    QFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open file: %1").arg(file.errorString()));
        return;
    }

    QXmlStreamReader xmlReader(&file);
    int currentRow = 0;
    totalTranscriptWords = 0;
    totalMispronouncedWords = 0;
    totalTaggedWords = 0;
    totalTranscriptEditedWords = 0;
    totalMispronouncedEditedWords = 0;
    totalTaggedEditedWords = 0;

    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        if (xmlReader.isStartElement() && xmlReader.name() == QString("row")) {
            TTSRow row;
            while (!(xmlReader.isEndElement() && xmlReader.name() == QString("row"))) {
                xmlReader.readNext();
                if (xmlReader.isStartElement()) {
                    QString elementName = QString::fromUtf8(xmlReader.name().toUtf8());
                    QXmlStreamAttributes attrs = xmlReader.attributes();

                    bool isEdited = attrs.hasAttribute("isEdited") &&
                                    attrs.value("isEdited").toString() == "true";

                    xmlReader.readNext();
                    QString text = QString::fromUtf8(xmlReader
                                       .text()
                                       .toUtf8()
                                    )
                                    .trimmed()
                                    .replace(Constants::Text::WHITESPACE_NORMALIZER, " ");

                    uint64_t totalWords = text.split(" ",  Qt::SkipEmptyParts).size();

                    if (elementName == QString("words")) {
                        row.words = text;
                        totalTranscriptWords += totalWords;
                        row.wordsEdited = isEdited;
                    } else if (elementName == QString("not-pronounced-properly")) {
                        row.not_pronounced_properly = text;
                        totalMispronouncedWords += totalWords;
                        row.pronunciationEdited = isEdited;
                    } else if (elementName == QString("sound-quality")) {
                        row.sound_quality = std::clamp(text.toInt(), 1, 5);
                    } else if (elementName == QString("asr-quality")) {
                        row.asr_quality = std::clamp(text.toInt(), 0, 1);
                    } else if (elementName == QString("audio-filename")) {
                        row.audioFileName = text;
                    } else if (elementName == QString("tag")) {
                        row.tag = text;
                        totalTaggedWords += text.trimmed().split(";", Qt::SkipEmptyParts).count();
                        row.tagEdited = isEdited;
                    }
                }
            }
            // Add the row and get its index
            int currentRow = m_model->addRow(row);
            m_model->storeOriginalData(currentRow, row.words,
                                       row.not_pronounced_properly,
                                       row.tag);

            // Set background colors for edited fields
            if (row.wordsEdited) {
                m_model->setData(m_model->index(currentRow, 1),
                                 QBrush(Qt::yellow), Qt::BackgroundRole);
            }
            if (row.pronunciationEdited) {
                m_model->setData(m_model->index(currentRow, 2),
                                 QBrush(Qt::yellow), Qt::BackgroundRole);
            }
            if (row.tagEdited) {
                m_model->setData(m_model->index(currentRow, 3),
                                 QBrush(Qt::yellow), Qt::BackgroundRole);
            }
        }
        xmlReader.readNext();
    }
    file.close();
    if (xmlReader.hasError()) {
        QMessageBox::warning(this, tr("XML Error"), tr("Error parsing XML: %1").arg(xmlReader.errorString()));
    }
    if (tableView) {
        loadWordCounts();
        tableView->viewport()->update();
    }

    ui->totalTranscriptWordsLbl->setText(QString("Total Transcript words: %1").arg(totalTranscriptWords));
    ui->totalMispronouncedWordsLbl->setText(QString("Total Mispronounced words: %1").arg(totalMispronouncedWords));
    ui->totalTaggedWordsLbl->setText(QString("Total Tagged words: %1").arg(totalTaggedWords));
}

void TTSAnnotator::save()
{
    if (fileUrl.isEmpty()) {
        saveAs();
    } else {
        saveToFile(fileUrl.toLocalFile());
    }
}

void TTSAnnotator::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), xmlDirectory, tr("XML Files (*.xml)"));
    if (!fileName.isEmpty()) {
        fileUrl = QUrl::fromLocalFile(fileName);
        xmlDirectory = QFileInfo(fileName).absolutePath();
        saveToFile(fileName);
    }
}

void TTSAnnotator::saveToFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open file for writing: %1").arg(file.errorString()));
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("transcript");

    const auto& rows = m_model->rows();
    for (const auto& row : rows) {
        xmlWriter.writeStartElement("row");

        // Write words with isEdited attribute
        xmlWriter.writeStartElement("words");
        xmlWriter.writeAttribute("isEdited", row.wordsEdited ? "true" : "false");
        xmlWriter.writeCharacters(row.words);
        xmlWriter.writeEndElement();

        // Write not-pronounced-properly with isEdited attribute
        xmlWriter.writeStartElement("not-pronounced-properly");
        xmlWriter.writeAttribute("isEdited", row.pronunciationEdited ? "true" : "false");
        xmlWriter.writeCharacters(row.not_pronounced_properly);
        xmlWriter.writeEndElement();

        xmlWriter.writeTextElement("sound-quality", QString::number(row.sound_quality));
        xmlWriter.writeTextElement("asr-quality", QString::number(row.asr_quality));
        xmlWriter.writeTextElement("audio-filename", row.audioFileName);

        // Write tag with isEdited attribute
        xmlWriter.writeStartElement("tag");
        xmlWriter.writeAttribute("isEdited", row.tagEdited ? "true" : "false");
        xmlWriter.writeCharacters(row.tag);
        xmlWriter.writeEndElement();

        xmlWriter.writeEndElement(); // row
    }

    xmlWriter.writeEndElement(); // transcript
    xmlWriter.writeEndDocument();

    file.close();

    if (file.error() != QFile::NoError) {
        QMessageBox::warning(this, tr("Save Error"), tr("Error occurred while saving the file: %1").arg(file.errorString()));
    } else {
        // updateEditedWordCounts();
        saveWordCounts();
        QMessageBox::information(this,
            tr("Save Successful"),
            tr("File saved successfully.\n"
               "Total Transcript words: %1\n"
               "Total Mispronounced words: %2\n"
               "Total Tagged words: %3\n"
               "Total Transcript edited words: %4\n"
               "Total Mispronounced edited words: %5\n"
               "Total Tagged edited words: %6\n")
                 .arg(totalTranscriptWords)
                 .arg(totalMispronouncedWords)
                 .arg(totalTaggedWords)
                 .arg(totalTranscriptEditedWords)
                 .arg(totalMispronouncedEditedWords)
                 .arg(totalTaggedEditedWords)
        );
    }
}

void TTSAnnotator::insertRow()
{
    m_model->insertRow(m_model->rowCount());
}

void TTSAnnotator::deleteRow()
{
    QModelIndex currentIndex = tableView->currentIndex();
    if (currentIndex.isValid()) {
        m_model->removeRow(currentIndex.row());
    }
}

void TTSAnnotator::on_saveAsTableButton_clicked()
{
    saveAs();
}

void TTSAnnotator::on_InsertRowButton_clicked()
{
    insertRow();
}

void TTSAnnotator::on_deleteRowButton_clicked()
{
    deleteRow();
}

void TTSAnnotator::on_saveTableButton_clicked()
{
    save();
}

void TTSAnnotator::on_actionOpen_triggered()
{
    openTTSTranscript();
}

void TTSAnnotator::onCellClicked(const QModelIndex &index)
{
    if (index.column() == 0) {  // Assuming audio player is in the first column
        tableView->openPersistentEditor(index);
    }
}

void TTSAnnotator::setDefaultFontOnTableView()
{
    // QFont defaultFont = tableView->font();

    // // Define a list of preferred fonts
    // QStringList preferredFonts = {
    //     ".AppleSystemUIFont",  // macOS system font
    //     "SF Pro",              // macOS
    //     "Segoe UI",            // Windows
    //     "Roboto",              // Android and modern systems
    //     "Noto Sans",           // Good Unicode coverage
    //     "Arial",               // Widely available
    //     "Helvetica"            // Fallback
    // };

    // QString chosenFont;
    // for (const QString& fontFamily : preferredFonts) {
    //     if (QFontDatabase::families().contains(fontFamily)) {
    //         chosenFont = fontFamily;
    //         break;
    //     }
    // }

    // if (!chosenFont.isEmpty()) {
    //     defaultFont.setFamily(chosenFont);
    // }

    // tableView->setFont(defaultFont);

    // tableView->resizeRowsToContents();
}

QString TTSAnnotator::getWordCountFilename() {
    if (fileUrl.isEmpty()) return QString();

    QDir baseDir(QFileInfo(fileUrl.toLocalFile()).absolutePath());
    // Create wordcount directory if it doesn't exist
    if (!baseDir.exists("wordcount")) {
        if (!baseDir.mkdir("wordcount")) {
            qDebug() << "Failed to create wordcount directory";
            QMessageBox::critical(this, "Error", "Failed to create the 'wordcount' directory.");
            return QString();
        }
    }

    QString baseName = QFileInfo(fileUrl.toLocalFile()).fileName();
    return baseDir.absolutePath() + "/wordcount/" + baseName.left(baseName.lastIndexOf('.')) + "_wordcount.xml";
}

void TTSAnnotator::saveWordCounts() {
    QString countFile = getWordCountFilename();
    if (countFile.isEmpty()) return;

    QFile file(countFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open word count file for writing:" << file.errorString();
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("wordcounts");

    xmlWriter.writeTextElement("transcript_edited", QString::number(totalTranscriptEditedWords));
    xmlWriter.writeTextElement("mispronounced_edited", QString::number(totalMispronouncedEditedWords));
    xmlWriter.writeTextElement("tagged_edited", QString::number(totalTaggedEditedWords));
    xmlWriter.writeTextElement("transcript_total", QString::number(totalTranscriptWords));
    xmlWriter.writeTextElement("mispronounced_total", QString::number(totalMispronouncedWords));
    xmlWriter.writeTextElement("tagged_total", QString::number(totalTaggedWords));

    xmlWriter.writeEndElement(); // wordcounts
    xmlWriter.writeEndDocument();
    file.close();
}

void TTSAnnotator::loadWordCounts() {
    QString countFile = getWordCountFilename();
    if (countFile.isEmpty()) return;

    QFile file(countFile);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        // Reset counts if file doesn't exist or can't be opened
        totalTranscriptEditedWords = 0;
        totalMispronouncedEditedWords = 0;
        totalTaggedEditedWords = 0;
        return;
    }

    QXmlStreamReader xmlReader(&file);
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        if (xmlReader.isStartElement()) {
            QString elementName = xmlReader.name().toString();
            xmlReader.readNext();

            if (elementName == "transcript_edited")
                totalTranscriptEditedWords = xmlReader.text().toString().toULongLong();
            else if (elementName == "mispronounced_edited")
                totalMispronouncedEditedWords = xmlReader.text().toString().toULongLong();
            else if (elementName == "tagged_edited")
                totalTaggedEditedWords = xmlReader.text().toString().toULongLong();
            else if (elementName == "transcript_total")
                totalTranscriptWords = xmlReader.text().toString().toULongLong();
            else if (elementName == "mispronounced_total")
                totalMispronouncedWords = xmlReader.text().toString().toULongLong();
            else if (elementName == "tagged_total")
                totalTaggedWords = xmlReader.text().toString().toULongLong();
        }
        xmlReader.readNext();
    }
    file.close();

    // Update UI
    ui->totalTranscriptEditedWordsLbl->setText(QString("Total Transcript edited words: %1").arg(totalTranscriptEditedWords));
    ui->totalMispronouncedEditedWordsLbl->setText(QString("Total Mispronounced edited words: %1").arg(totalMispronouncedEditedWords));
    ui->totalTaggedEditedWordsLbl->setText(QString("Total Tagged edited words: %1").arg(totalTaggedEditedWords));
}
