#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QStringList>

/*!
 * \brief A word list that is searched on disk instead of being loaded into memory.
 *
 * The backing file is produced at build time by the \c dictbuild tool: UTF-8, one word
 * per line, sorted by byte value and deduplicated. Because it is already sorted, this
 * class can memory-map it and binary search the mapped bytes directly. Nothing is
 * copied onto the heap and nothing is sorted at run time.
 *
 * That replaces the previous approach of reading every word into a QStringList. For
 * Tamil that meant 1.27 million QStrings, several hundred megabytes of resident memory
 * once UTF-16 conversion and per-string overhead are counted, plus a full sort on the
 * UI thread every time the transcript language changed.
 *
 * Words added at run time (custom dictionaries, words the user marked as correct) live
 * in a small sorted overlay in memory, so the mapped file never has to be rewritten.
 */
class Dictionary
{
public:
    Dictionary() = default;
    ~Dictionary();

    Dictionary(const Dictionary&) = delete;
    Dictionary& operator=(const Dictionary&) = delete;

    /*!
     * \brief Locates and maps the dictionary for a language.
     *
     * Searches the directories returned by searchPaths() for "<language>.dict".
     *
     * \return True when a file was mapped. A dictionary that fails to open still works,
     *         it simply contains nothing but the overlay.
     */
    bool load(const QString& language);

    /*! \brief Releases the mapping. */
    void close();

    /*! \brief True when a backing file is currently mapped. */
    bool isLoaded() const { return m_map != nullptr; }

    /*! \brief True when the word is in the mapped file or in the overlay. */
    bool contains(const QString& word) const;

    /*!
     * \brief Adds a word to the in-memory overlay.
     *
     * The overlay is kept sorted so lookups stay logarithmic. The mapped file is
     * untouched, so this is cheap regardless of how large the dictionary is.
     */
    void addWord(const QString& word);

    /*! \brief Adds several words to the overlay in one pass. */
    void addWords(const QStringList& words);

    /*! \brief Empties the overlay, leaving the mapped file alone. */
    void clearOverlay();

    /*!
     * \brief Returns up to \a limit words starting with \a prefix.
     *
     * Used to feed the completer a small model per keystroke rather than handing it
     * every word in the language.
     */
    QStringList completions(const QString& prefix, int limit = 100) const;

    /*! \brief Directories that are searched for dictionary files, in order. */
    static QStringList searchPaths();

private:
    /*!
     * \brief Byte offset of the first line that is not less than \a needle.
     *
     * Classic binary search over a newline delimited file: probe the midpoint, walk
     * back to the start of that line, compare, and narrow. Returns m_size when every
     * line sorts before \a needle.
     */
    qint64 lowerBound(const QByteArray& needle) const;

    /*! \brief The line beginning at \a offset, without its newline. */
    QByteArray lineAt(qint64 offset, qint64* lineEnd = nullptr) const;

    QFile m_file;
    uchar* m_map = nullptr;
    qint64 m_size = 0;
    QStringList m_overlay; ///< Sorted. Custom dictionary entries and corrected words.
};
