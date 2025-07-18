#pragma once

#include "editor/plaintexteditwithcompleter.h"
#include <QStyledItemDelegate>
#include <QComboBox>
#include "qstyleditemdelegate.h"


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
    void showDropdownOnEditStart(QAbstractItemView* view, const QModelIndex& index) const;

private:
    int m_min, m_max;
    QColor m_color;
};


#include <QStyledItemDelegate>
#include <QPlainTextEdit>

class CustomTextEdit : public PlainTextEditWithCompleter {
    Q_OBJECT
public:
    explicit CustomTextEdit(QWidget *parent = nullptr);
    void setOriginalText(const QString& text) { m_originalText = text; }
    QString originalText() const { return m_originalText; }
protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
private:
    QCompleter *m_textCompleter = nullptr;
    QString m_originalText;
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
    QColor m_color = QColor(255, 255, 255);

signals:
    void transcriptWordsEdited(int64_t wordCount);
    void mispronunciationWordsEdited(int64_t wordCount);
    void tagWordsEdited(int64_t wordCount);

private:
    int calculateChangedWords(const QString& newValue, const QString& originalValue, const QString& delimiter) const;;
};

class CheckBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    CheckBoxDelegate(bool checked, const QColor& color, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
private:
    QColor m_color;
    bool m_checked;
};

class CheckableComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit CheckableComboBox(QWidget* parent = nullptr);

    QStringList checkedItems() const;
    void setCheckedItems(const QStringList& items);

private:
    void updateText();

private slots:
    // void itemStateChanged(const QModelIndex& index);
};

class CheckableComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CheckableComboBoxDelegate(const QStringList& options, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
private:
    QStringList m_options;
};
