#include "customdelegates.h"
#include "audioplayer/audioplayerwidget.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include "editor/texteditor.h"
#include "qdir.h"

AudioPlayerDelegate::AudioPlayerDelegate(const QString& baseDir, QObject* parent)
    : QStyledItemDelegate(parent), m_baseDir(baseDir)
{
}

AudioPlayerDelegate::~AudioPlayerDelegate()
{
    qDeleteAll(m_activeEditors);
}

QWidget* AudioPlayerDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)

    // Stop all other players
    stopAllPlayers();

    // Clean up unused editors
    cleanupUnusedEditors();

    // Create new editor if it doesn't exist
    if (!m_activeEditors.contains(index)) {
        QString fileName = index.model()->data(index, Qt::EditRole).toString();
        QString filePath = m_baseDir + QDir::separator() + fileName;
        AudioPlayerWidget* editor = new AudioPlayerWidget(filePath, parent);
        editor->setAutoFillBackground(true);
        m_activeEditors[index] = editor;
    }

    m_lastPlayingIndex = index;
    return m_activeEditors[index];
}

void AudioPlayerDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
    // We don't need to set editor data here because we're creating a new AudioPlayerWidget
    // with the correct file path in createEditor
}

void AudioPlayerDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    AudioPlayerWidget* audioPlayer = qobject_cast<AudioPlayerWidget*>(editor);
    if (audioPlayer) {
        QString fileName = audioPlayer->getAudioFileName(true);
        model->setData(index, fileName, Qt::EditRole);
    }
}

void AudioPlayerDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

void AudioPlayerDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (m_activeEditors.contains(index)) {
        // If there's an active editor for this index, don't paint anything
        return;
    }

    // Otherwise, paint the default text
    QStyledItemDelegate::paint(painter, option, index);
}

void AudioPlayerDelegate::stopAllPlayers() const
{
    for (AudioPlayerWidget* player : m_activeEditors) {
        if (player) {
            player->stop();
        }
    }
}

void AudioPlayerDelegate::cleanupUnusedEditors() const
{
    QMutableMapIterator<QModelIndex, AudioPlayerWidget*> i(m_activeEditors);
    while (i.hasNext()) {
        i.next();
        if (!i.key().isValid() || i.key() != m_lastPlayingIndex) {
            if (i.value()) {
                i.value()->stop();
                delete i.value();
            }
            i.remove();
        }
    }
}

void AudioPlayerDelegate::clearAllEditors()
{
    stopAllPlayers();

    // Delete all editors
    for (auto* editor : m_activeEditors) {
        delete editor;
    }
    m_activeEditors.clear();
    m_lastPlayingIndex = QModelIndex();
}

void AudioPlayerDelegate::setBaseDir(QString pBaseDir)
{
    if (m_baseDir != pBaseDir) {
        // Stop and clear all players before changing directory
        clearAllEditors();
        m_baseDir = pBaseDir;
    }
}

ComboBoxDelegate::ComboBoxDelegate(int min, int max, const QColor& color, QObject* parent)
    : QStyledItemDelegate(parent), m_min(min), m_max(max), m_color(color)
{
}

QWidget* ComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    QComboBox* editor = new QComboBox(parent);
    for (int i = m_min; i <= m_max; ++i) {
        editor->addItem(QString::number(i));
    }

    QString styleSheet = QString(
                             "QComboBox {"
                             "   background-color: %1;"
                             "   selection-background-color: %2;"
                             "   selection-color: black;"
                             "}"
                             "QComboBox QAbstractItemView {"
                             "   background-color: %1;"
                             "   selection-background-color: %2;"
                             "   selection-color: black;"
                             "}"
                         ).arg(m_color.name()).arg(m_color.darker(110).name());
    editor->setStyleSheet(styleSheet);
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    if (comboBox) {
        int value = index.model()->data(index, Qt::EditRole).toInt();
        comboBox->setCurrentIndex(value - m_min);
    }
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    if (comboBox) {
        model->setData(index, comboBox->currentText().toInt(), Qt::EditRole);
    }
}

void ComboBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    // Fill the background with the specified color
    painter->fillRect(option.rect, m_color);

    // Draw the text
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(option.palette.color(QPalette::Text));
    painter->drawText(option.rect, Qt::AlignCenter, text);

    // Draw the focus rect if the item has focus
    if (option.state & QStyle::State_HasFocus) {
        QStyleOptionFocusRect focusOption;
        focusOption.rect = option.rect;
        focusOption.state = option.state | QStyle::State_KeyboardFocusChange | QStyle::State_Item;
        focusOption.backgroundColor = option.palette.color(QPalette::Base);
        QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, painter);
    }

    painter->restore();
}

TextEditDelegate::TextEditDelegate(QFont font, QWidget *parent)
    : QStyledItemDelegate(parent), m_font(font)
{
}

QWidget *TextEditDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    QPlainTextEdit *editor = new QPlainTextEdit(parent);
    editor->document()->setDefaultFont(m_font);
    return editor;
}

void TextEditDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();
    CustomTextEdit *textEdit = static_cast<CustomTextEdit*>(editor);
    textEdit->setPlainText(value);
}

void TextEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    CustomTextEdit *textEdit = static_cast<CustomTextEdit*>(editor);
    QString value = textEdit->toPlainText();
    model->setData(index, value, Qt::EditRole);
}

void TextEditDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

void TextEditDelegate::setFont(QFont font)
{
    m_font = font;
}

CustomTextEdit::CustomTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
}

void CustomTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (e->modifiers() & Qt::ShiftModifier) {
            // Shift+Enter inserts a new line
            QPlainTextEdit::keyPressEvent(e);
        } else {
            // Enter without shift finishes editing
            e->accept();
            QWidget *editor = this;
            QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(editor->parent());
            if (delegate) {
                emit delegate->commitData(editor);
                emit delegate->closeEditor(editor);
            }
        }
    } else if (e->key() == Qt::Key_Escape) {
        e->accept();
        QWidget *editor = this;
        QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(editor->parent());
        if (delegate) {
            emit delegate->closeEditor(editor);
        }
    } else {
        QPlainTextEdit::keyPressEvent(e);
    }
}

void CustomTextEdit::focusOutEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusOutEvent(e);

    QWidget *editor = this;
    QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(editor->parent());
    if (delegate) {
        emit delegate->commitData(editor);
        emit delegate->closeEditor(editor, QAbstractItemDelegate::NoHint);
    }
}
