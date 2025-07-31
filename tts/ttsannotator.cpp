#include "ttsannotator.h"
#include "audioplayer/audioplayerwidget.h"
#include "tts/utilities/findandreplacedialog.h"
#include "ui_ttsannotator.h"
#include "lazyloadingmodel.h"
#include "customdelegates.h"
#include "utils/constants.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <qshortcut.h>
#include <qtimer.h>

const QColor TTSAnnotator::SoundQualityColor = QColor(230, 255, 230);
const QColor TTSAnnotator::TTSQualityColor = QColor(255, 230, 230);

TTSAnnotator::TTSAnnotator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TTSAnnotator)
    , m_model(std::make_unique<LazyLoadingModel>())
    , m_undoStack(std::make_unique<QUndoStack>(this))
    , m_saveTimer(new QTimer(this))
{
    ui->setupUi(this);
    tableView = ui->tableView;
    tableView->setModel(m_model.get());
    setDefaultFontOnTableView();
    tableView->installEventFilter(this);
    setupUI();

    // QString iniPath = QApplication::applicationDirPath() + "/" + "config.ini";
    settings = std::make_unique<QSettings>(Constants::Vagyojaka::CONFIG_INI, QSettings::IniFormat);

    this->supportedFormats = {
        "xml Files (*.xml)",
        "All Files (*)"
    };
    m_model->setUndoStack(m_undoStack.get());
    setupShortcuts();
}

TTSAnnotator::~TTSAnnotator() = default;

void TTSAnnotator::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    const auto &deselectedIndexes = deselected.indexes();
    for (const QModelIndex &index : deselectedIndexes) {
        if (index.column() == 0) {
            tableView->closePersistentEditor(index);
        }
    }
    const auto &selectedIndexes = selected.indexes();
    for (const QModelIndex &index : selectedIndexes) {
        if (index.column() == 0) {
            tableView->openPersistentEditor(index);
        }
    }
}

void TTSAnnotator::setupUI()
{
    tableView->setFocusPolicy(Qt::StrongFocus);
    // Set headers for the model
    m_model->setHorizontalHeaderLabels({
        "Audios", "Hypothesis", "Transcript", "Tags", "Comments", "WER"/*, "Sound Quality", "ASR Quality"*/
    });

    // Set up delegates
    if (!m_audioPlayerDelegate) {
        m_audioPlayerDelegate = new AudioPlayerDelegate(xmlDirectory, this);
    }
    tableView->setItemDelegateForColumn(0, m_audioPlayerDelegate);
    // ComboBoxDelegate* soundQualityDelegate = new ComboBoxDelegate(1, 5, SoundQualityColor.darker(105), this);
    // ComboBoxDelegate* ttsQualityDelegate = new ComboBoxDelegate(0, 1, TTSQualityColor.darker(105), this);
    CheckableComboBoxDelegate* checkableComboBoxDelegate = new CheckableComboBoxDelegate({"Start Not Matching", "End Not Matching", "Resegment", "Divided Audio"}, this);

    // Add TextEditDelegate for text columns
    textDelegate = new TextEditDelegate(font(), this);
    tableView->setItemDelegateForColumn(1, textDelegate); // Hypothesis column
    tableView->setItemDelegateForColumn(2, textDelegate); // Transcript column
    tableView->setItemDelegateForColumn(3, checkableComboBoxDelegate); // Tags column
    tableView->setItemDelegateForColumn(4, textDelegate); // Comments column

    // soundQualityDelegate->
    // tableView->setItemDelegateForColumn(4, soundQualityDelegate);
    // tableView->setItemDelegateForColumn(5, ttsQualityDelegate);

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

    // Set initial focus
    tableView->setFocus();

    // Resize rows and columns to content
    tableView->resizeRowsToContents();
    // tableView->resizeColumnsToContents();
    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TTSAnnotator::onItemSelectionChanged);

    connect(m_saveTimer, &QTimer::timeout, this, [this]() {
        if (m_autoSave && fileUrl.isValid())
            save();
    });
    m_saveTimer->start(m_saveInterval * 1000);
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
    tableView->setFocus();
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
        // Strech the columns to fit content
        tableView->resizeColumnsToContents();
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
        xmlReader.readNext();

        if (xmlReader.isStartElement() && xmlReader.name() == QStringLiteral("row")) {
            TTSRow row;

            while (!(xmlReader.isEndElement() && xmlReader.name() == QStringLiteral("row"))) {
                xmlReader.readNext();

                if (xmlReader.isStartElement()) {
                    QString elementName = xmlReader.name().toString();
                    QXmlStreamAttributes attributes = xmlReader.attributes();

                    // Read the text content after the start element
                    xmlReader.readNext();
                    QString text = xmlReader.text().toString();

                    if (elementName == "words") {
                        row.words = text;
                        if (attributes.hasAttribute("isEdited"))
                            row.wordsEdited = (attributes.value("isEdited").toString() == "1");
                    } else if (elementName == "comments") {
                        row.comments = text;
                        if (attributes.hasAttribute("isEdited"))
                            row.commentsEdited = (attributes.value("isEdited").toString() == "1");
                    } else if (elementName == "audio-filename") {
                        row.audioFileName = text;
                    } else if (elementName == "tag") {
                        row.tags = text;
                        if (attributes.hasAttribute("isEdited"))
                            row.tagsEdited = (attributes.value("isEdited").toString() == "1");
                    } else if (elementName == "wer") {
                        row.wer = text;
                        bool ok = false;
                        double werValue = text.toDouble(&ok);
                        if (ok && werValue >= 0.1) {
                            row.markAsHighWER = true;
                        }
                    } else if (elementName == "hypothesis") {
                        row.hypothesis = text;
                    }
                }
            }

            int newRowIndex = m_model->addRow(row);

            m_model->storeOriginalData(newRowIndex, row.words, row.tags, row.comments);

            for (int col = 0; col < m_model->columnCount(); ++col) {
                QModelIndex index = m_model->index(newRowIndex, col);
                m_model->setData(index, Constants::Brush::Peppermint, Qt::BackgroundRole);
            }

            const auto& yellowBrush = Constants::Brush::Yellow;
            const auto& peppermintBursh = Constants::Brush::Peppermint;
            const auto& appleBrush = Constants::Brush::Apple;
            const auto& azelea = Constants::Brush::Azalea;
            const auto& froly = Constants::Brush::Froly;

            QBrush brush = appleBrush;
            if (row.markAsHighWER) {
                for (int col = 0; col < m_model->columnCount(); ++col) {
                    QModelIndex index = m_model->index(newRowIndex, col);
                    m_model->setData(index, azelea, Qt::BackgroundRole);
                }
                brush = froly;
            }

            if (row.wordsEdited) {
                QModelIndex wordsIndex = m_model->index(newRowIndex, 2);
                m_model->setData(wordsIndex, brush, Qt::BackgroundRole);
            }

            if (row.tagsEdited) {
                QModelIndex tagsIndex = m_model->index(newRowIndex, 3);
                m_model->setData(tagsIndex, brush, Qt::BackgroundRole);
            }

            if (row.commentsEdited) {
                QModelIndex commentsIndex = m_model->index(newRowIndex, 4);
                m_model->setData(commentsIndex, brush, Qt::BackgroundRole);
            }

        }
    }
    file.close();
    if (xmlReader.hasError()) {
        QMessageBox::warning(this, tr("XML Error"), tr("Error parsing XML: %1").arg(xmlReader.errorString()));
    }
    m_undoStack->clear();
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

        xmlWriter.writeTextElement("hypothesis", row.hypothesis);

        xmlWriter.writeStartElement("words");
        xmlWriter.writeAttribute("isEdited", row.wordsEdited ? "1" : "0");
        xmlWriter.writeCharacters(row.words);
        xmlWriter.writeEndElement();

        xmlWriter.writeTextElement("audio-filename", row.audioFileName);

        xmlWriter.writeStartElement("tag");
        xmlWriter.writeAttribute("isEdited", row.tagsEdited ? "1" : "0");
        xmlWriter.writeCharacters(row.tags);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("comments");
        xmlWriter.writeAttribute("isEdited", row.commentsEdited ? "1" : "0");
        xmlWriter.writeCharacters(row.comments);
        xmlWriter.writeEndElement();

        xmlWriter.writeTextElement("wer", row.wer);

        xmlWriter.writeEndElement(); // row
    }

    xmlWriter.writeEndElement(); // transcript
    xmlWriter.writeEndDocument();

    file.close();

    if (file.error() != QFile::NoError) {
        QMessageBox::warning(this, tr("Save Error"), tr("Error occurred while saving the file: %1").arg(file.errorString()));
    } else if (!m_autoSave) {
        QMessageBox::information(this, tr("Save Successful"), tr("File saved successfully."));
    }
    m_undoStack->setClean();
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
    if (index.column() == 0) {
        tableView->openPersistentEditor(index);
    }
    tableView->setFocus();
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

void TTSAnnotator::setupShortcuts() {
    QShortcut* playShortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Space), this);
    connect(playShortcut, &QShortcut::activated, this, &TTSAnnotator::toggleCurrentAudioPlayer);\

    QShortcut* undoShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), this);
    undoShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(undoShortcut, &QShortcut::activated, this, &TTSAnnotator::onUndo);

    QShortcut* redoShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Y), this);
    redoShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(redoShortcut, &QShortcut::activated, this, &TTSAnnotator::onRedo);
}

void TTSAnnotator::toggleCurrentAudioPlayer() {
    QModelIndex current = tableView->currentIndex();
    if (!current.isValid()) return;

    QModelIndex audioIndex = current.siblingAtColumn(0);
    tableView->openPersistentEditor(audioIndex);

    if (AudioPlayerDelegate* delegate = qobject_cast<AudioPlayerDelegate*>(tableView->itemDelegateForColumn(0))) {
        if (AudioPlayerWidget* player = delegate->getActivePlayer(audioIndex)) {
            delegate->stopActivePlayer(player);
            player->togglePlayPause();
        }
    }
}

void TTSAnnotator::undo() {
    m_undoStack->undo();
}

void TTSAnnotator::redo() {
    m_undoStack->redo();
}

void TTSAnnotator::onUndo() {
    undo();
}

void TTSAnnotator::onRedo() {
    redo();
}
