#pragma once

#include "tts/ttsrow.h"
#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>


struct OriginalData {
    QString words;
    QString notPronouncedProperly;
    QString tags;
};

struct EditedCount {
    int editedWords = 0;
    int editedNotPronouncedProperlyWords = 0;
    int edittedTaggedWords = 0;
};

class LazyLoadingModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit LazyLoadingModel(QObject* parent = nullptr);

    struct EditInfo {
        QString currentText;
        int wordCount;
        bool isActive;  // false if reverted to original
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int addRow(const TTSRow& row);
    void insertRow(int row);
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());
    void clear();
    const QVector<TTSRow>& rows() const;
    void setTransliterate(bool flag, const QString& langCode = "en");
    const OriginalData getOriginalData(int row) const;

    void setHorizontalHeaderLabels(const QStringList& labels);
    bool transliterate { false };
    QString transliterateLangCode = "en";

    void storeOriginalData(int row, const QString& words, const QString& notPronounced, const QString& tags);
    QVector<EditInfo> getEditHistory(int row, int column) const;
    int getTotalEditedWords(int column) const;
    void clearEditHistory(int row, int column);
    int calculateColumnEditedWords(int column) const;
    QStringList& getDropdownCheckboxOpts();

signals:
    void transcriptEditedWordsCount(int64_t count);
    void mispronouncedEditedWordsCount(int64_t count);
    void taggedEditedWordsCount(int64_t count);

private:
    QVector<TTSRow> m_rows;
    QStringList m_horizontalHeaderLabels;

    QMap<QModelIndex, QBrush> m_backgroundColors;
    QMap<int, OriginalData> m_originalData;
    QMap<QPair<int, int>, QVector<EditInfo>> m_editHistory;

    int calculateWordDifference(const QString& newText, const QString& originalText, const QString& delimiter);
    void updateEditHistory(int row, int column, const QString& newText, const QString& delimiter);
    bool isRevertedToOriginal(int row, int column, const QString& newText);
    int calculateChangedWords(const QString& current, const QString& original, const QString& delimiter) const;
    void init();

    QHash<int, EditedCount> editedCounts;
    OriginalData emptyOriginalData;
    QStringList chkbxDropdownOpts;
};
