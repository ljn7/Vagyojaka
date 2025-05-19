#pragma once

#include "tts/ttsrow.h"
#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>

class LazyLoadingModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit LazyLoadingModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addRow(const TTSRow& row);
    void insertRow(int row);
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());
    void clear();
    const QVector<TTSRow>& rows() const;
    void setTransliterate(bool flag, const QString& langCode = "en");

    void setHorizontalHeaderLabels(const QStringList& labels);
    bool transliterate { false };
    QString transliterateLangCode = "en";

private:
    QVector<TTSRow> m_rows;
    QStringList m_horizontalHeaderLabels;
};
