// Standalone check of Dictionary's on-disk binary search against the real .dict files.
#include "editor/dictionary.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QRandomGenerator>

static int g_failures = 0;

static void check(bool ok, const QString& what)
{
    if (!ok) {
        qWarning().noquote() << "FAIL:" << what;
        ++g_failures;
    }
}

static QStringList readAll(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll()).split('\n', Qt::SkipEmptyParts);
}

static void testLanguage(const QString& language, const QString& dictPath)
{
    const QStringList words = readAll(dictPath);
    if (words.isEmpty()) {
        qWarning().noquote() << "FAIL: could not read" << dictPath;
        ++g_failures;
        return;
    }

    Dictionary dict;
    check(dict.load(language), language + ": load");
    check(dict.isLoaded(), language + ": isLoaded");

    // Every word in the file must be found. Sample across the whole range, and always
    // include the first and last entries since those exercise the search boundaries.
    QList<qsizetype> indices{0, words.size() - 1, words.size() / 2};
    for (int i = 0; i < 2000; ++i)
        indices << QRandomGenerator::global()->bounded(qint64(words.size()));

    int notFound = 0;
    for (qsizetype i : indices) {
        if (!dict.contains(words.at(i))) {
            if (notFound < 5)
                qWarning().noquote() << "  missing:" << words.at(i) << "at index" << i;
            ++notFound;
        }
    }
    check(notFound == 0, QStringLiteral("%1: all sampled words found (%2 misses)").arg(language).arg(notFound));

    // Words that are not in the file must not be reported as present.
    const QStringList absent{QStringLiteral("zzzzqqqxnotaword"), QStringLiteral("!!!!notaword!!!!"),
                             QStringLiteral(""), QStringLiteral("\u0000zz")};
    int falsePositives = 0;
    for (const QString& w : absent)
        if (dict.contains(w))
            ++falsePositives;
    check(falsePositives == 0, language + ": no false positives");

    // Ordering sanity: a prefix search must return words that actually carry the prefix.
    const QString sample = words.at(words.size() / 3);
    const QString prefix = sample.left(qMin(3, int(sample.size())));
    const QStringList completions = dict.completions(prefix, 25);
    check(!completions.isEmpty(), QStringLiteral("%1: completions for '%2'").arg(language, prefix));
    bool allMatch = true;
    for (const QString& c : completions)
        if (!c.startsWith(prefix))
            allMatch = false;
    check(allMatch, language + ": all completions carry the prefix");

    // The overlay must be searched alongside the mapped file.
    const QString invented = QStringLiteral("zzzzqqqxnotaword");
    dict.addWord(invented);
    check(dict.contains(invented), language + ": overlay word found");
    check(dict.completions(QStringLiteral("zzzzqqq"), 5).contains(invented), language + ": overlay completion");
    dict.clearOverlay();
    check(!dict.contains(invented), language + ": overlay cleared");

    qInfo().noquote() << language << "checked against" << words.size() << "words";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/wordlists/");
    testLanguage(QStringLiteral("english"), dir + QStringLiteral("english.dict"));
    testLanguage(QStringLiteral("tamil"), dir + QStringLiteral("tamil.dict"));
    testLanguage(QStringLiteral("sanskrit"), dir + QStringLiteral("sanskrit.dict"));

    Dictionary missing;
    check(!missing.load(QStringLiteral("klingon")), "missing language fails to load");
    check(!missing.contains(QStringLiteral("anything")), "missing language contains nothing");
    missing.addWord(QStringLiteral("qapla"));
    check(missing.contains(QStringLiteral("qapla")), "overlay works without a mapped file");

    if (g_failures == 0) {
        qInfo().noquote() << "\nALL DICTIONARY CHECKS PASSED";
        return 0;
    }
    qWarning().noquote() << "\n" << g_failures << "CHECK(S) FAILED";
    return 1;
}
