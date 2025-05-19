#include "lazyloadingmodel.h"
#include "qcolor.h"
#include "tts/ttsannotator.h"

LazyLoadingModel::LazyLoadingModel(QObject* parent)
    : QAbstractTableModel(parent)
{
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
        switch (index.column()) {
        case 4: // Sound Quality column
            return TTSAnnotator::SoundQualityColor; // Light green
        case 5: // TTS Quality column
            return TTSAnnotator::TTSQualityColor; // Light red
        default:
            return QVariant(); // Default background
        }
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return row.audioFileName;
        case 1: return row.words;
        case 2: return row.not_pronounced_properly;
        case 3: return row.tag;
        case 4: return row.sound_quality;
        case 5: return row.asr_quality;
        }
    }

    if (role == Qt::UserRole + 1  && index.column() == 2) {
        QVariantMap result;
        result["transliterate"] = transliterate;
        result["transliterateLangCode"] = transliterateLangCode;

        return result;
    }

    return QVariant();
}

bool LazyLoadingModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        TTSRow& row = m_rows[index.row()];
        switch (index.column()) {
        case 1: row.words = value.toString(); break;
        case 2: row.not_pronounced_properly = value.toString(); break;
        case 3: row.tag = value.toString(); break;
        case 4: row.sound_quality = value.toInt(); break;
        case 5: row.asr_quality = value.toInt(); break;
        default: return false;
        }
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
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

void LazyLoadingModel::addRow(const TTSRow& row)
{
    beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
    m_rows.append(row);
    endInsertRows();
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

