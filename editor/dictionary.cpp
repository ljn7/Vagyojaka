#include "dictionary.h"

#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <algorithm>
#include <cstring>

namespace {
Q_LOGGING_CATEGORY(lcDict, "vagyojaka.dictionary")

/*!
 * \brief Byte order comparison, matching what dictbuild sorted the file with.
 *
 * memcmp compares as unsigned char, which is also how std::char_traits<char> orders
 * string_view. Using QByteArray::compare or QString comparison here instead would
 * order some bytes differently and the binary search would miss words.
 */
int byteCompare(const char* a, qsizetype aLen, const char* b, qsizetype bLen)
{
    const qsizetype common = std::min(aLen, bLen);
    if (common > 0) {
        const int c = std::memcmp(a, b, static_cast<size_t>(common));
        if (c != 0)
            return c;
    }
    if (aLen == bLen)
        return 0;
    return aLen < bLen ? -1 : 1;
}
} // namespace

Dictionary::~Dictionary()
{
    close();
}

QStringList Dictionary::searchPaths()
{
    const QString appDir = QCoreApplication::applicationDirPath();

    QStringList paths{
        appDir + QStringLiteral("/wordlists"),
        // macOS application bundle.
        appDir + QStringLiteral("/../Resources/wordlists"),
        // Linux install prefix.
        appDir + QStringLiteral("/../share/vagyojaka/wordlists"),
    };

    for (const QString& dir : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation))
        paths << dir + QStringLiteral("/wordlists");

    return paths;
}

bool Dictionary::load(const QString& language)
{
    close();

    if (language.isEmpty())
        return false;

    const QString fileName = language + QStringLiteral(".dict");

    for (const QString& dir : searchPaths()) {
        const QString candidate = QDir(dir).filePath(fileName);
        if (!QFile::exists(candidate))
            continue;

        m_file.setFileName(candidate);
        if (!m_file.open(QIODevice::ReadOnly)) {
            qCWarning(lcDict) << "Cannot open" << candidate << m_file.errorString();
            continue;
        }

        m_size = m_file.size();
        if (m_size <= 0) {
            m_file.close();
            continue;
        }

        m_map = m_file.map(0, m_size);

        // The file handle is only needed for the mapping itself. Closing it here keeps
        // the descriptor count down; the mapping stays valid.
        m_file.close();

        if (!m_map) {
            qCWarning(lcDict) << "Cannot map" << candidate;
            m_size = 0;
            continue;
        }

        qCInfo(lcDict) << "Mapped" << candidate << m_size << "bytes";
        return true;
    }

    qCWarning(lcDict) << "No dictionary found for" << language << "in" << searchPaths();
    return false;
}

void Dictionary::close()
{
    if (m_map) {
        m_file.unmap(m_map);
        m_map = nullptr;
    }
    if (m_file.isOpen())
        m_file.close();
    m_size = 0;
}

QByteArray Dictionary::lineAt(qint64 offset, qint64* lineEnd) const
{
    qint64 end = offset;
    while (end < m_size && m_map[end] != '\n')
        ++end;

    if (lineEnd)
        *lineEnd = end;

    return QByteArray::fromRawData(reinterpret_cast<const char*>(m_map + offset),
                                   static_cast<qsizetype>(end - offset));
}

qint64 Dictionary::lowerBound(const QByteArray& needle) const
{
    qint64 lo = 0;
    qint64 hi = m_size;

    // lo is always the first byte of a line, which is what makes the backward walk
    // below terminate at a line boundary rather than running off the start.
    while (lo < hi) {
        const qint64 mid = lo + (hi - lo) / 2;

        qint64 lineStart = mid;
        while (lineStart > lo && m_map[lineStart - 1] != '\n')
            --lineStart;

        qint64 lineEnd = 0;
        const QByteArray line = lineAt(lineStart, &lineEnd);

        if (byteCompare(line.constData(), line.size(), needle.constData(), needle.size()) < 0)
            lo = lineEnd + 1; // Strictly greater than mid, so this always makes progress.
        else
            hi = lineStart;
    }

    return lo;
}

bool Dictionary::contains(const QString& word) const
{
    if (word.isEmpty())
        return false;

    if (std::binary_search(m_overlay.cbegin(), m_overlay.cend(), word))
        return true;

    if (!m_map)
        return false;

    const QByteArray needle = word.toUtf8();
    const qint64 at = lowerBound(needle);
    if (at >= m_size)
        return false;

    const QByteArray line = lineAt(at);
    return byteCompare(line.constData(), line.size(), needle.constData(), needle.size()) == 0;
}

void Dictionary::addWord(const QString& word)
{
    if (word.isEmpty())
        return;

    const auto at = std::lower_bound(m_overlay.begin(), m_overlay.end(), word);
    if (at != m_overlay.end() && *at == word)
        return;

    m_overlay.insert(at, word);
}

void Dictionary::addWords(const QStringList& words)
{
    if (words.isEmpty())
        return;

    m_overlay += words;
    m_overlay.sort();
    m_overlay.removeDuplicates();
}

void Dictionary::clearOverlay()
{
    m_overlay.clear();
}

QStringList Dictionary::completions(const QString& prefix, int limit) const
{
    QStringList out;
    if (prefix.isEmpty() || limit <= 0)
        return out;

    // Overlay first, so a word the user just marked as correct shows up immediately.
    for (auto it = std::lower_bound(m_overlay.cbegin(), m_overlay.cend(), prefix);
         it != m_overlay.cend() && out.size() < limit; ++it) {
        if (!it->startsWith(prefix))
            break;
        out << *it;
    }

    if (!m_map)
        return out;

    const QByteArray needle = prefix.toUtf8();
    qint64 at = lowerBound(needle);

    while (at < m_size && out.size() < limit) {
        qint64 lineEnd = 0;
        const QByteArray line = lineAt(at, &lineEnd);

        if (!line.startsWith(needle))
            break;

        out << QString::fromUtf8(line);
        at = lineEnd + 1;
    }

    out.sort();
    out.removeDuplicates();
    return out;
}
