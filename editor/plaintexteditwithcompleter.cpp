#include "plaintexteditwithcompleter.h"
#include <qabstractitemview.h>
#include <qeventloop.h>
#include <qstringlistmodel.h>
#include <qtextobject.h>
#include <qtimer.h>
#include <QScrollBar>


PlainTextEditWithCompleter::PlainTextEditWithCompleter(QWidget *parent)
    : QPlainTextEdit(parent), m_transliterationCompleter(makeCompleter())
{
    m_transliterationCompleter->setModel(new QStringListModel);
    connect(m_transliterationCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, &PlainTextEditWithCompleter::insertTransliterationCompletion);
}

QCompleter* PlainTextEditWithCompleter::makeCompleter() {
    auto completer = new QCompleter(this);
    completer->setWidget(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    completer->setCompletionMode(QCompleter::PopupCompletion);

    return completer;
}

void PlainTextEditWithCompleter::keyPressEvent(QKeyEvent* event) {
    auto checkPopupVisible = [](QCompleter* completer) {
        return completer && completer->popup()->isVisible();
    };

    if (checkPopupVisible(m_transliterationCompleter)) {
        switch (event->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                //       case Qt::Key_Space:
                event->ignore();
                return; // let the completer do default behavior
            default:
                break;
        }
    }

    QPlainTextEdit::keyPressEvent(event);

    QString blockText = textCursor().block().text();
    QString textTillCursor = blockText.left(textCursor().positionInBlock());
    QString completionPrefix;

    const bool shortcutPressed = (event->key() == Qt::Key_N && event->modifiers() == Qt::ControlModifier);
    const bool hasModifier = (event->modifiers() != Qt::NoModifier);
    const bool containsSpeakerBraces = blockText.left(blockText.indexOf(" ")).contains("}:");

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word

    if (!shortcutPressed && (hasModifier || event->text().isEmpty() || eow.contains(event->text().right(1)))) {
        m_transliterationCompleter->popup()->hide();
        return;
    }

    int index = textTillCursor.count(" ");
    completionPrefix = blockText.split(" ")[index];

    if (completionPrefix.isEmpty()){
        m_transliterationCompleter->popup()->hide();
        return;
    }

    QCompleter *m_completer = nullptr;

    if (m_transliterate) {
        m_completer = m_transliterationCompleter;
    }

    if (!m_completer)
        return;


    QTimer replyTimer;
    replyTimer.setSingleShot(true);
    QEventLoop loop;
    connect(this, &PlainTextEditWithCompleter::replyCame, &loop, &QEventLoop::quit);
    connect(&replyTimer, &QTimer::timeout, &loop,
            [&]() {
                emit message("Reply Timeout, Network Connection is slow or inaccessible", 2000);
                loop.quit();
            });

    sendRequest(completionPrefix, m_transliterateLangCode);
    replyTimer.start(1000);
    loop.exec();

    dynamic_cast<QStringListModel*>(m_completer->model())->setStringList(m_lastReplyList);

    m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));

    QRect cr = cursorRect();
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(cr);
}

void PlainTextEditWithCompleter::sendRequest(const QString& input, const QString& langCode)
{
    if (m_reply) {
        m_reply->abort();
        delete m_reply;
        m_reply = nullptr;
    }

    QString url = QString("http://inputtools.google.com/request?text=%1&itc=%2-t-i0-und&num=10&cp=0&cs=1&ie=utf-8&oe=utf-8&app=test").arg(input, langCode);

    QNetworkRequest request(url);
    m_reply = m_manager.get(request);

    connect(m_reply, &QNetworkReply::finished, this, [this] () {
        if (m_reply->error() == QNetworkReply::NoError)
            handleReply();
        else if (m_reply->error() != QNetworkReply::OperationCanceledError)
            emit message(m_reply->errorString());
        emit replyCame();
    });
}

void PlainTextEditWithCompleter::handleReply()
{
    QStringList tokens;

    QString replyString = m_reply->readAll();

    if (replyString.split("[\"").size() < 4)
        return;

    tokens = replyString.split("[\"")[3].split("]").first().split("\",\"");

    auto lastString = tokens[tokens.size() - 1];
    tokens[tokens.size() - 1] = lastString.left(lastString.size() - 1);

    m_lastReplyList = tokens;
}

void PlainTextEditWithCompleter::useTransliteration(bool value, const QString& langCode)
{
    m_transliterate = value;
    m_transliterateLangCode = langCode;
}

void PlainTextEditWithCompleter::insertTransliterationCompletion(const QString& completion)
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    tc.insertText(completion);

    setTextCursor(tc);
}
