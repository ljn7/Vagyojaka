#pragma once

#include <QStyledItemDelegate>
#include <QComboBox>


class AudioPlayerWidget;
class QModelIndex;

class AudioPlayerDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    AudioPlayerDelegate(const QString& baseDir, QObject* parent = nullptr);
    ~AudioPlayerDelegate() override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void clearAllEditors();
    void stopAllPlayers() const;
    void cleanupUnusedEditors() const;
    void setBaseDir(QString pBaseDir);

private:
    QString m_baseDir;
    mutable QMap<QModelIndex, AudioPlayerWidget*> m_activeEditors;
    mutable QModelIndex m_lastPlayingIndex;

};

class ComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    ComboBoxDelegate(int min, int max, const QColor& color, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int m_min, m_max;
    QColor m_color;
};
