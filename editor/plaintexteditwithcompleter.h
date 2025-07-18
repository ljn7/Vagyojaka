#ifndef PLAINTEXTEDITWITHCOMPLETER_H
#define PLAINTEXTEDITWITHCOMPLETER_H

#include <QPlainTextEdit>
#include <qcompleter.h>
#include <qnetworkreply.h>

class PlainTextEditWithCompleter : public QPlainTextEdit
{
    Q_OBJECT
public:
    PlainTextEditWithCompleter(QWidget *parent = nullptr);
    void useTransliteration(bool value, const QString& langCode);

signals:
    void message(const QString& text, int timeout = 5000);
    void replyCame();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    QCompleter* m_transliterationCompleter = nullptr;

private:
    QCompleter* makeCompleter();
    void handleReply();
    void sendRequest(const QString& input, const QString& langCode);
    void insertTransliterationCompletion(const QString& completion);


    QNetworkReply* m_reply = nullptr;
    QStringList m_lastReplyList;
    QNetworkAccessManager m_manager;
    bool m_transliterate { false };
    QString m_transliterateLangCode = "en";
};

#endif // PLAINTEXTEDITWITHCOMPLETER_H
