#include "ttsannotator.h"
#include "tts/utilities/findandreplacedialog.h"
#include "ui_ttsannotator.h"
#include "lazyloadingmodel.h"
#include "customdelegates.h"
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
    ComboBoxDelegate* ttsQualityDelegate = new ComboBoxDelegate(0, 1, TTSQualityColor.darker(105), this);

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
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // tableView->horizontalHeader()->viewport()->update();

    // Store the column widths after stretch
    QVector<int> columnWidths;
    for (int i = 0; i < tableView->model()->columnCount(); ++i) {
        columnWidths.append(tableView->columnWidth(i));
    }
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    // Restore the stretched widths for columns
    for (int i = 0; i < columnWidths.size(); ++i) {
        tableView->setColumnWidth(i, columnWidths[i]);
    }

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
    // connect(ui->InsertRowButton, &QPushButton::clicked, this, &TTSAnnotator::insertRow);
    // connect(ui->deleteRowButton, &QPushButton::clicked, this, &TTSAnnotator::deleteRow);
    // connect(ui->saveAsTableButton, &QPushButton::clicked, this, &TTSAnnotator::saveAs);
    // connect(ui->saveTableButton, &QPushButton::clicked, this, &TTSAnnotator::save);

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

    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        if (xmlReader.isStartElement() && xmlReader.name() == QString("row")) {
            TTSRow row;
            while (!(xmlReader.isEndElement() && xmlReader.name() == QString("row"))) {
                xmlReader.readNext();
                if (xmlReader.isStartElement()) {
                    QString elementName = QString::fromUtf8(xmlReader.name().toUtf8());
                    xmlReader.readNext();
                    QString text = QString::fromUtf8(xmlReader.text().toUtf8());
                    if (elementName == QString("words")) {
                        row.words = text;
                    } else if (elementName == QString("not-pronounced-properly")) {
                        row.not_pronounced_properly = text;
                    } else if (elementName == QString("sound-quality")) {
                        row.sound_quality = text.toInt();
                    } else if (elementName == QString("asr-quality")) {
                        row.asr_quality = text.toInt();
                    } else if (elementName == QString("audio-filename")) {
                        row.audioFileName = text;
                    } else if (elementName == QString("tag")) {
                        row.tag = text;
                    }
                }
            }
            m_model->addRow(row);
        }
        xmlReader.readNext();
    }
    file.close();
    if (xmlReader.hasError()) {
        QMessageBox::warning(this, tr("XML Error"), tr("Error parsing XML: %1").arg(xmlReader.errorString()));
    }
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
        xmlWriter.writeTextElement("words", row.words);
        xmlWriter.writeTextElement("not-pronounced-properly", row.not_pronounced_properly);
        xmlWriter.writeTextElement("sound-quality", QString::number(row.sound_quality));
        xmlWriter.writeTextElement("asr-quality", QString::number(row.asr_quality));
        xmlWriter.writeTextElement("audio-filename", row.audioFileName);
        xmlWriter.writeTextElement("tag", row.tag);
        xmlWriter.writeEndElement(); // row
    }

    xmlWriter.writeEndElement(); // transcript
    xmlWriter.writeEndDocument();

    file.close();

    if (file.error() != QFile::NoError) {
        QMessageBox::warning(this, tr("Save Error"), tr("Error occurred while saving the file: %1").arg(file.errorString()));
    } else {
        QMessageBox::information(this, tr("Save Successful"), tr("File saved successfully."));
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

void TTSAnnotator::openFindReplaceDialog()
{
    FindAndReplaceDialog *dialog = new FindAndReplaceDialog(tableView, this);
    dialog->exec();
    delete dialog;
}

void TTSAnnotator::useTransliteration(bool flag, const QString &langCode) {
    m_model->setTransliterate(flag, langCode);
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
