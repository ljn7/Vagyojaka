#pragma once

#include "qplaintextedit.h"
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


#include <QStyledItemDelegate>
#include <QPlainTextEdit>

class CustomTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CustomTextEdit(QWidget *parent = nullptr);
protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
};

class TextEditDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit TextEditDelegate( QFont font, QWidget *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

    void setFont(QFont font);
    QFont m_font;
};

class VerificationDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit VerificationDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QRect checkboxRect(const QStyleOptionViewItem& option) const;
    void drawVerificationState(QPainter* painter, const QRect& rect, int state) const;
};
