#pragma once

#include <QList>
#include <QMultiMap>
#include <QSyntaxHighlighter>

/*!
 * \brief Applies transcript specific formatting: spell check underlines, tag
 *        colouring, edited word backgrounds and the currently played line.
 *
 * Extracted from editor.h, where it lived alongside the editor widget despite
 * sharing no state with it.
 */
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {};

    /*!
     * rief Suspends rehighlighting until the matching endUpdate().
     *
     * Every setter below used to rehighlight the whole document on its own. A single
     * edit calls seven of them, so one keystroke triggered seven full passes over the
     * transcript. Wrapping a batch in beginUpdate()/endUpdate() collapses that to one.
     * Nested calls are counted, so the guard composes.
     */
    void beginUpdate()
    {
        ++updateDepth;
    }

    /*! rief Ends a batch and performs the deferred rehighlight, if one was requested. */
    void endUpdate()
    {
        if (updateDepth == 0)
            return;
        if (--updateDepth == 0 && rehighlightPending) {
            rehighlightPending = false;
            rehighlight();
        }
    }

    void clearHighlight()
    {
        blockToHighlight = -1;
        wordToHighlight = -1;
    }
    void setBlockToHighlight(qint64 blockNumber)
    {
        blockToHighlight = blockNumber;
        requestRehighlight();
    }
    void setWordToHighlight(int wordNumber)
    {
        wordToHighlight = wordNumber;
        requestRehighlight();
    }
    void setInvalidBlocks(const QList<int>& invalidBlocks)
    {
        invalidBlockNumbers = invalidBlocks;
        requestRehighlight();
    }
    void setTaggedBlocks(const QList<int>& taggedBlock)
    {
        taggedBlockNumbers = taggedBlock;
        requestRehighlight();
    }
    void clearTaggedBlocks()
    {
        taggedBlockNumbers.clear();
    }
    void setInvalidWords(const QMultiMap<int, int>& invalidWordsMap)
    {
        invalidWords = invalidWordsMap;
        requestRehighlight();
    }
    void setTaggedWords(const QMultiMap<int, int>& taggedWordsMap)
    {
        taggedWords = taggedWordsMap;
        requestRehighlight();
    }
    void setEditedWords(const QMultiMap<int, int>& editedWordsMap) {
        editedWords = editedWordsMap;
        requestRehighlight();
    }
    void clearInvalidBlocks()
    {
        invalidBlockNumbers.clear();
    }

    void highlightBlock(const QString&) override;

private:
    /*! rief Rehighlights now, or marks it pending when inside a batch. */
    void requestRehighlight()
    {
        if (updateDepth > 0) {
            rehighlightPending = true;
            return;
        }
        rehighlight();
    }

    int blockToHighlight{-1};
    int wordToHighlight{-1};
    QList<int> invalidBlockNumbers;
    QList<int> taggedBlockNumbers;

    QMultiMap<int, int> invalidWords;
    QMultiMap<int, int> taggedWords;
    QMultiMap<int, int> editedWords;

    int updateDepth{0};
    bool rehighlightPending{false};
};

/*!
 * rief Scope guard that batches a run of Highlighter setters into one rehighlight.
 */
class HighlighterUpdate
{
public:
    explicit HighlighterUpdate(Highlighter* highlighter) : m_highlighter(highlighter)
    {
        if (m_highlighter)
            m_highlighter->beginUpdate();
    }

    ~HighlighterUpdate()
    {
        if (m_highlighter)
            m_highlighter->endUpdate();
    }

    HighlighterUpdate(const HighlighterUpdate&) = delete;
    HighlighterUpdate& operator=(const HighlighterUpdate&) = delete;

private:
    Highlighter* m_highlighter;
};
