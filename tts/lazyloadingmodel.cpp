#include "lazyloadingmodel.h"
#include "qcolor.h"
#include "tts/ttsannotator.h"
#include "utils/constants.h"


class LazyLoadingModel::EditCommand : public QUndoCommand {
public:
    EditCommand(LazyLoadingModel* model, const QModelIndex& index,
                const QVariant& oldValue, const QVariant& newValue)
        : m_model(model), m_index(index),
        m_oldValue(oldValue), m_newValue(newValue) {}

    void undo() override {
        m_model->m_isUndoing = true;
        m_model->setData(m_index, m_oldValue, Qt::EditRole);
        m_model->m_isUndoing = false;
    }

    void redo() override {
        m_model->m_isUndoing = true;
        m_model->setData(m_index, m_newValue, Qt::EditRole);
        m_model->m_isUndoing = false;
    }

private:
    LazyLoadingModel* m_model;
    QPersistentModelIndex m_index;
    QVariant m_oldValue;
    QVariant m_newValue;
};

LazyLoadingModel::LazyLoadingModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    init();
}

void LazyLoadingModel::init() {
    emptyOriginalData.words = "";
    emptyOriginalData.tags = "";
    emptyOriginalData.comments = "";

    chkbxDropdownOpts = QStringList({"Start Not Matching", "End Not Matching", "Resegment", "Divided Audio"});
}
int LazyLoadingModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

int LazyLoadingModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 6;
}

QVariant LazyLoadingModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size() || index.column() >= 6)
        return QVariant();

    const TTSRow& row = m_rows[index.row()];

    if (role == Qt::BackgroundRole) {

        if (index.column() >= 0 && index.column() <= 6) {
            if (m_backgroundColors.contains(index)) {
                return m_backgroundColors[index];
            }
            return QVariant(); // Default background
        }
        // switch (index.column()) {
        //     case 4: // Sound Quality column
        //         return TTSAnnotator::SoundQualityColor; // Light green
        //     case 5: // TTS Quality column
        //         return TTSAnnotator::TTSQualityColor; // Light red
        //     default:
        //         return QVariant(); // Default background
        // }
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case 0: return row.audioFileName;
            case 1: return row.hypothesis;
            case 2: return row.words;
            case 3: return row.tags;
            case 4: return row.comments;
            case 5: return row.wer;
            // case 4: return row.sound_quality;
            // case 5: return row.asr_quality;
        }
    }

    if (role == Qt::UserRole + 4  && (index.column() == 2 || index.column() == 4)) {
        QVariantMap result;
        result["transliterate"] = transliterate;
        result["transliterateLangCode"] = transliterateLangCode;

        return result;
    }

    return QVariant();
}

Qt::ItemFlags LazyLoadingModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant LazyLoadingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section >= 0 && section < m_horizontalHeaderLabels.size()) {
            return m_horizontalHeaderLabels.at(section);
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int LazyLoadingModel::addRow(const TTSRow& row)
{
    beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
    m_rows.append(row);

    int newRow = m_rows.size() - 1;
    endInsertRows();

    return newRow;
}

void LazyLoadingModel::insertRow(int row)
{
    beginInsertRows(QModelIndex(), row, row);
    m_rows.insert(row, TTSRow());
    endInsertRows();
}

bool LazyLoadingModel::removeRow(int row, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row);
    m_rows.removeAt(row);
    endRemoveRows();
    return true;
}

void LazyLoadingModel::clear()
{
    beginResetModel();
    m_rows.clear();
    m_backgroundColors.clear();
    m_originalData.clear();
    m_editHistory.clear();
    endResetModel();
}

const QVector<TTSRow>& LazyLoadingModel::rows() const
{
    return m_rows;
}

void LazyLoadingModel::setHorizontalHeaderLabels(const QStringList& labels)
{
    m_horizontalHeaderLabels = labels;
    emit headerDataChanged(Qt::Horizontal, 0, labels.size() - 1);
}

void LazyLoadingModel::setTransliterate(bool flag, const QString& langCode) {
    transliterate = flag;
    transliterateLangCode = langCode;
}

int LazyLoadingModel::calculateChangedWords(const QString& current, const QString& original, const QString& delimiter) const {
    if (current.isNull() || original.isNull() || delimiter.isEmpty()) {
        return 0; // Return 0 changes if inputs are invalid
    }

    QStringList currentWords = current.split(delimiter, Qt::SkipEmptyParts);
    QStringList originalWords = original.split(delimiter, Qt::SkipEmptyParts);

    // We only need two rows: current and previous
    QVector<int> prevRow(currentWords.size() + 1);
    QVector<int> currRow(currentWords.size() + 1);

    // Initialize first row
    for (int j = 0; j <= currentWords.size(); j++) {
        prevRow[j] = j;
    }

    // Fill the dp table using only two rows
    for (int i = 1; i <= originalWords.size(); i++) {
        currRow[0] = i;

        for (int j = 1; j <= currentWords.size(); j++) {
            if (originalWords[i-1] == currentWords[j-1]) {
                currRow[j] = prevRow[j-1];
            } else {
                currRow[j] = 1 + std::min({prevRow[j],    // deletion
                                           currRow[j-1],  // insertion
                                           prevRow[j-1]}); // substitution
            }
        }

        prevRow.swap(currRow);
    }

    return prevRow[currentWords.size()];
}

int LazyLoadingModel::calculateColumnEditedWords(int column) const {
    int totalEdited = 0;
    QString delimiter = (column == 3) ? ";" : " ";  // Use semicolon for tags, space for other text

    // Go through each row and compare with original data
    for (int row = 0; row < m_rows.size(); ++row) {
        const OriginalData original = getOriginalData(row);
        const TTSRow& current = m_rows[row];

        QString originalText;
        QString currentText;

        switch (column) {
        case 2: // Words column
            originalText = original.words;
            currentText = current.words;
            break;
        case 3: // Not pronounced properly column
            originalText = original.tags;
            currentText = current.tags;
            break;
        case 4: // Tags column
            originalText = original.comments;
            currentText = current.comments;
            break;
        default:
            continue;
        }

        totalEdited += calculateChangedWords(currentText, originalText, delimiter);
    }

    return totalEdited;
}

void LazyLoadingModel::storeOriginalData(int row, const QString& words,
                                         const QString& tags,
                                         const QString& comments) {
    OriginalData data;
    data.words = words;
    data.tags = tags;
    data.comments = comments;
    m_originalData[row] = data;
}

const OriginalData LazyLoadingModel::getOriginalData(int row) const {
    // Defensive check for valid row
    if (row < -1 || row >= m_rows.size()) {
        return emptyOriginalData;  // Return empty data for invalid row
    }

    auto it = m_originalData.find(row);
    if (it == m_originalData.end()) {
        return emptyOriginalData;  // Return empty data if not found
    }

    return it.value();
}


int LazyLoadingModel::calculateWordDifference(const QString& newText, const QString& originalText, const QString& delimiter) {
    QStringList newWords = newText.split(delimiter, Qt::SkipEmptyParts);
    QStringList originalWords = originalText.split(delimiter, Qt::SkipEmptyParts);

    // Create sets from lists using construction instead of fromList
    QSet<QString> newSet(newWords.begin(), newWords.end());
    QSet<QString> originalSet(originalWords.begin(), originalWords.end());

    // Words added or modified
    QSet<QString> addedWords = newSet.subtract(originalSet);

    // Words removed
    QSet<QString> removedWords = originalSet.subtract(newSet);

    return addedWords.size() + removedWords.size();
}

void LazyLoadingModel::updateEditHistory(int row, int column, const QString& newText, const QString& delimiter) {
    QPair<int, int> cellKey(row, column);
    const OriginalData originalData = getOriginalData(row);
    QString originalText;

    switch(column) {
        case 2: originalText = originalData.words; break;
        case 3: originalText = originalData.tags; break;
        case 4: originalText = originalData.comments; break;
        default: return;
    }

    EditInfo newEdit;
    newEdit.currentText = newText;
    newEdit.wordCount = calculateWordDifference(newText, originalText, delimiter);
    newEdit.isActive = true;

    if (!m_editHistory.contains(cellKey)) {
        m_editHistory[cellKey] = QVector<EditInfo>();
    }

    // If reverting to original, mark previous edits as inactive
    if (newText == originalText) {
        for (EditInfo& edit : m_editHistory[cellKey]) {
            edit.isActive = false;
        }
    }

    m_editHistory[cellKey].append(newEdit);
}

bool LazyLoadingModel::isRevertedToOriginal(int row, int column, const QString& newText) {
    const OriginalData originalData = getOriginalData(row);
    switch(column) {
        case 2: return newText == originalData.words;
        case 3: return newText == originalData.tags;
        case 4: return newText == originalData.comments;
        default: return false;
    }
}

bool LazyLoadingModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= m_rows.size())
        return false;

    if (role == Qt::BackgroundRole) {
        m_backgroundColors[index] = value.value<QBrush>();
        emit dataChanged(index, index, {role});
        return true;
    }


    TTSRow& ttsRow = m_rows[index.row()];

    // Handle the isEdited flags
    if (role == Qt::UserRole + 2) {
        ttsRow.wordsEdited = value.toBool();
        emit dataChanged(index, index, {role});
        return true;
    }
    if (role == Qt::UserRole + 3) {
        ttsRow.tagsEdited = value.toBool();
        emit dataChanged(index, index, {role});
        return true;
    }
    if (role == Qt::UserRole + 4) {
        ttsRow.commentsEdited = value.toBool();
        emit dataChanged(index, index, {role});
        return true;
    }

    if (role == Qt::EditRole) {

        TTSRow& ttsRow = m_rows[index.row()];
        QVariant oldValue;

        switch (index.column()) {
        case 2: oldValue = ttsRow.words; break;
        case 3: oldValue = ttsRow.tags; break;
        case 4: oldValue = ttsRow.comments; break;
        default: return false;
        }

        // Create undo command if not in undo/redo operation
        if (!m_isUndoing && m_undoStack) {
            m_undoStack->push(new EditCommand(this, index, oldValue, value));
        }

        QString newText = value.toString().trimmed();
        int row = index.row();
        int column = index.column();

        if (column <= 1 || column == 5) {
            return false;
        }

        // Only process text columns
        if (column >= 2 && column <= 4) {
            QString delimiter = (column == 3) ? ";" : " ";
            const OriginalData originalData = getOriginalData(row);
            // Get the appropriate original text based on column
            QString originalText;

            switch (column) {
                case 2: originalText = originalData.words; break;
                case 3: originalText = originalData.tags; break;
                case 4: originalText = originalData.comments; break;
                default: return false;
            }
            // Calculate word difference before updating the model

            // Determine if this edit reverts to original
            bool isReverted = (newText == originalText);
            int wordDiff = 0;
            // Update edited flags and background colors
            if (isReverted) {
                if (row < m_rows.size() && m_rows[row].markAsHighWER) {
                    setData(index, Constants::Brush::Azalea, Qt::BackgroundRole);
                } else {
                    setData(index, Constants::Brush::Peppermint, Qt::BackgroundRole);
                }
                setData(index, false, Qt::UserRole + column);

                // Signal negative word count to remove previous edits
                switch (column) {
                case 2:
                    wordDiff = editedCounts[row].editedWords;
                    editedCounts[row].editedWords = 0;
                    if (wordDiff > 0) {
                        emit transcriptEditedWordsCount(-wordDiff);
                    }
                    break;
                case 3:
                    wordDiff = editedCounts[row].editedTaggedWords;
                    editedCounts[row].editedTaggedWords = 0;
                    if (wordDiff > 0) {
                        emit taggedEditedWordsCount(-wordDiff);
                    }
                    break;
                case 4:
                    wordDiff = editedCounts[row].editedComments;
                    editedCounts[row].editedComments = 0;
                    if (wordDiff > 0) {
                        emit commentsEditedWordsCount(-wordDiff);
                    }
                    break;
                }
            } else {
                wordDiff = calculateChangedWords(newText, originalText, delimiter);
                QString currentColor = m_backgroundColors.value(index, QBrush()).color().name();

                if (currentColor == Constants::Colors::Azalea.name() || ttsRow.markAsHighWER >= 0.1) {
                    setData(index, Constants::Brush::Froly, Qt::BackgroundRole);
                } else {
                    setData(index, Constants::Brush::Apple, Qt::BackgroundRole);
                }
                setData(index, true, Qt::UserRole + column);

                // Signal positive word count for new edits
                switch (column) {
                case 2:
                    if (editedCounts[row].editedWords != wordDiff)
                    {
                        int diff = wordDiff - editedCounts[row].editedWords;
                        editedCounts[row].editedWords = wordDiff;
                        emit transcriptEditedWordsCount(diff);
                    }
                    break;
                case 3:
                    if (editedCounts[row].editedTaggedWords != wordDiff) {
                        int diff = wordDiff - editedCounts[row].editedTaggedWords;
                        editedCounts[row].editedTaggedWords = wordDiff;
                        emit taggedEditedWordsCount(diff);
                    }
                    break;
                case 4:
                    if (editedCounts[row].editedComments != wordDiff) {
                        int diff = wordDiff - editedCounts[row].editedComments;
                        editedCounts[row].editedComments = wordDiff;
                        emit commentsEditedWordsCount(diff);
                    }
                    break;
                }
            }
        }
        // Update the actual data in the model
        switch (index.column()) {
        case 2:
            ttsRow.words = newText;
            break;
        case 3:
            ttsRow.tags = newText;
            break;
        case 4:
            ttsRow.comments = newText;
            break;
        default:
            return false;
        }
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

QVector<LazyLoadingModel::EditInfo> LazyLoadingModel::getEditHistory(int row, int column) const {
    QPair<int, int> cellKey(row, column);
    return m_editHistory.value(cellKey);
}

int LazyLoadingModel::getTotalEditedWords(int column) const {
    int total = 0;
    for (auto it = m_editHistory.constBegin(); it != m_editHistory.constEnd(); ++it) {
        if (it.key().second == column) {
            // Only count active edits
            for (const EditInfo& edit : it.value()) {
                if (edit.isActive) {
                    total += edit.wordCount;
                }
            }
        }
    }
    return total;
}

QStringList &LazyLoadingModel::getDropdownCheckboxOpts()  {
    return chkbxDropdownOpts;
}

void LazyLoadingModel::setUndoStack(QUndoStack* stack) {
    m_undoStack = stack;
}
