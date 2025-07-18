#include "customdelegates.h"
#include "audioplayer/audioplayerwidget.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <iostream>
#include <qabstractitemview.h>
#include <qlineedit.h>
#include <qobjectcleanuphandler.h>
#include <qstandarditemmodel.h>
#include <qtimer.h>
#include "editor/texteditor.h"
#include "qdir.h"
#include "qcheckbox.h"
#include "tts/lazyloadingmodel.h"
#include "utils/constants.h"

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

void ComboBoxDelegate::showDropdownOnEditStart(QAbstractItemView* view, const QModelIndex& index) const
{
    // Create and show editor
    QWidget* editor = createEditor(view->viewport(), QStyleOptionViewItem(), index);
    view->setIndexWidget(index, editor);

    // Cast to QComboBox
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    if (comboBox) {
        // Set the data
        setEditorData(editor, index);

        // Show dropdown automatically
        comboBox->showPopup();

        // Connect signals for when editing is complete
        connect(comboBox, QOverload<int>::of(&QComboBox::activated), [=]() {
            setModelData(editor, const_cast<QAbstractItemModel*>(index.model()), index);
            view->setIndexWidget(index, nullptr);
            delete editor;
        });

        // Handle focus out
        comboBox->installEventFilter(new QObjectCleanupHandler());
        comboBox->setFocusPolicy(Qt::StrongFocus);
        comboBox->setFocus();
    }
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

    CustomTextEdit *editor = new CustomTextEdit(parent);

    if (!editor)
        return nullptr;

    if (index.column() == 2) {
        QVariant variant = index.model()->data(index, Qt::UserRole + 4);
        QVariantMap dataMap = variant.toMap();

        bool useTransliterate = dataMap.value("transliterate").toBool();
        QString transliterateLangCode = dataMap.value("transliterateLangCode").toString();

        editor->useTransliteration(useTransliterate, transliterateLangCode);
    }

    editor->document()->setDefaultFont(m_font);
    if (index.column() == 1) {
        editor->setReadOnly(true);
    }
    // Set center alignment through the document's text option
    QTextOption textOption = editor->document()->defaultTextOption();
    textOption.setAlignment(Qt::AlignCenter);
    editor->document()->setDefaultTextOption(textOption);

    // Style the editor
    QString styleSheet = QString(
                             "CustomTextEdit {"
                             "   border: none;"
                             "   background-color: %1;"
                             "   selection-background-color: %2;"
                             "   padding: 2px 5px;"
                             "}"
                        ).arg(m_color.name()).arg(m_color.darker(110).name());

    editor->setStyleSheet(styleSheet);
    return editor;
}

void TextEditDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
{
    if (!editor) {
        return;
    }
    QString value = index.model()->data(index, Qt::EditRole).toString();
    CustomTextEdit *textEdit = qobject_cast<CustomTextEdit*>(editor);
    if (!textEdit) {
        return;
    }

    QString text = value.trimmed().replace(Constants::Text::WHITESPACE_NORMALIZER, " ");
    textEdit->setPlainText(value);
    textEdit->setOriginalText(text);
}

void TextEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    if (CustomTextEdit *textEdit = qobject_cast<CustomTextEdit*>(editor)) {
        QString newValue = textEdit->toPlainText().trimmed();
        if (textEdit->originalText() == newValue)
            return;
        LazyLoadingModel* lazyModel = qobject_cast<LazyLoadingModel*>(model);
        if (!lazyModel) return;
        model->setData(index, newValue, Qt::EditRole);
    }
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

int TextEditDelegate::calculateChangedWords(const QString& newValue, const QString& originalValue, const QString& delimiter) const {
    QStringList newWords = newValue.split(delimiter, Qt::SkipEmptyParts);
    QStringList originalWords = originalValue.split(delimiter, Qt::SkipEmptyParts);

    int changedWords = 0;
    if (newWords.size() > originalWords.size()) {
        changedWords += newWords.size() - originalWords.size();
        for (const QString& word : originalWords) {
            if (!newWords.contains(word)) changedWords++;
        }
    } else if (newWords.size() < originalWords.size()) {
        changedWords += originalWords.size() - newWords.size();
        for (const QString& word : newWords) {
            if (!originalWords.contains(word)) changedWords++;
        }
    } else {
        for (const QString& word : newWords) {
            if (!originalWords.contains(word)) changedWords++;
        }
    }
    return changedWords;
}

CustomTextEdit::CustomTextEdit(QWidget *parent)
    : PlainTextEditWithCompleter(parent)
{
}

void CustomTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (m_transliterationCompleter && m_transliterationCompleter->popup()->isVisible()) {
        PlainTextEditWithCompleter::keyPressEvent(e);
        return;
    }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QWidget *editor = this;
        QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(editor->parent());
        if (delegate) {
            // emit delegate->commitData(editor);
            emit delegate->closeEditor(editor);
        }
        e->accept();
    } else if (e->key() == Qt::Key_Escape) {
        QWidget *editor = this;
        QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(editor->parent());
        if (delegate) {
            emit delegate->commitData(editor);
            emit delegate->closeEditor(editor);
        }
        e->accept();
    } else {
        PlainTextEditWithCompleter::keyPressEvent(e);
    }

}

void CustomTextEdit::focusOutEvent(QFocusEvent *e)
{
    PlainTextEditWithCompleter::focusOutEvent(e);

    QWidget *editor = this;
    QStyledItemDelegate *delegate = qobject_cast<QStyledItemDelegate*>(editor->parent());
    if (delegate) {
        emit delegate->commitData(editor);
        emit delegate->closeEditor(editor, QAbstractItemDelegate::NoHint);
    }
}

CheckBoxDelegate::CheckBoxDelegate(bool checked, const QColor& color, QObject* parent)
    : QStyledItemDelegate(parent), m_checked(checked), m_color(color)
{
}

QWidget* CheckBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    QCheckBox* editor = new QCheckBox(parent);
    bool initialState = index.data(Qt::EditRole).toBool();
    editor->setChecked(initialState);

    QString styleSheet = QString(
                             "QCheckBox {"
                             "   background-color: %1;"
                             "}"
                             "QCheckBox:checked {"
                             "   background-color: %2;"
                             "}")
                             .arg(m_color.name())
                             .arg(m_color.darker(110).name());
    editor->setStyleSheet(styleSheet);
    return editor;
}

void CheckBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor);
    if (checkBox) {
        bool checked = index.model()->data(index, Qt::EditRole).toBool();
        checkBox->setChecked(checked);
    }
}

void CheckBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(editor);
    if (checkBox) {
        model->setData(index, checkBox->isChecked(), Qt::EditRole);
    }
}

void CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    // Fill the background with the specified color
    QColor bgColor = m_color;
    if (option.state & QStyle::State_Selected) {
        bgColor = bgColor.darker(110);
    }

    painter->fillRect(option.rect, m_color);

    // Draw the checkbox
    QStyleOptionButton checkBoxOption;
    checkBoxOption.rect = option.rect;
    checkBoxOption.state = QStyle::State_Enabled;
    if (index.data(Qt::EditRole).toBool()) {
        checkBoxOption.state |= QStyle::State_On;
    } else {
        checkBoxOption.state |= QStyle::State_Off;
    }

    // Add hover effect
    if (option.state & QStyle::State_MouseOver) {
        checkBoxOption.state |= QStyle::State_MouseOver;
    }

    // Center the checkbox in the cell
    QRect indicatorRect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxOption);
    // checkBoxOption.rect.moveCenter(option.rect.center()); -- edited

    // Calculate the centered position for the checkbox
    QPoint center = option.rect.center();
    checkBoxOption.rect = QRect(
        center.x() - (indicatorRect.width() / 2),
        center.y() - (indicatorRect.height() / 2),
        indicatorRect.width(),
        indicatorRect.height()
        );


    QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, painter);

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

bool CheckBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonRelease ||
        event->type() == QEvent::MouseButtonDblClick)
    {
        bool currentState = index.data(Qt::EditRole).toBool();
        model->setData(index, !currentState, Qt::EditRole);
        return true;
    }
    return false;
}

CheckableComboBox::CheckableComboBox(QWidget* parent)
    : QComboBox(parent)
{
    setModel(new QStandardItemModel(this));
    connect(model(), &QAbstractItemModel::dataChanged, this, [this]() {
        updateText();
    });
    setEditable(true);
    lineEdit()->setReadOnly(true);
    lineEdit()->setPlaceholderText("Select options");
}

QStringList CheckableComboBox::checkedItems() const {
    QStringList selected;
    for (int i = 0; i < count(); ++i) {
        QStandardItem* item = qobject_cast<QStandardItemModel*>(model())->item(i);
        if (item->checkState() == Qt::Checked) {
            selected << item->text();
        }
    }
    return selected;
}

void CheckableComboBox::setCheckedItems(const QStringList& items) {
    for (int i = 0; i < count(); ++i) {
        QStandardItem* item = qobject_cast<QStandardItemModel*>(model())->item(i);
        if (items.contains(item->text())) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
    updateText();
}

void CheckableComboBox::updateText() {
    setEditText(checkedItems().join(";"));
}


CheckableComboBoxDelegate::CheckableComboBoxDelegate(const QStringList& options, QObject* parent)
    : QStyledItemDelegate(parent), m_options(options)
{
}

QWidget* CheckableComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&,
                                                 const QModelIndex& modelIndex) const {
    CheckableComboBox* editor = new CheckableComboBox(parent);
    editor->setEditable(true);
    auto* model = (LazyLoadingModel* )(modelIndex.model());
    QStringList opts;
    if (model) {
        opts = model->getDropdownCheckboxOpts();
    }
    for (const QString& option : opts) {
        QStandardItem* item = new QStandardItem(option);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        static_cast<QStandardItemModel*>(editor->model())->appendRow(item);
    }
    QTimer::singleShot(100, editor, [editor]() {
        if (editor) {
            editor->showPopup();
        }
    });
    return editor;
}

void CheckableComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    CheckableComboBox* comboBox = qobject_cast<CheckableComboBox*>(editor);
    if (!comboBox) return;

    QString data = index.model()->data(index, Qt::EditRole).toString();
    QStringList selectedItems = data.split(";", Qt::SkipEmptyParts);
    comboBox->setCheckedItems(selectedItems);
}

void CheckableComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                             const QModelIndex& index) const {
    CheckableComboBox* comboBox = qobject_cast<CheckableComboBox*>(editor);
    if (!comboBox) return;

    QStringList checked = comboBox->checkedItems();
    model->setData(index, checked.join(";"), Qt::EditRole);
}

void CheckableComboBoxDelegate::updateEditorGeometry(QWidget* editor,
                                                     const QStyleOptionViewItem& option,
                                                     const QModelIndex&) const {
    editor->setGeometry(option.rect);
}
