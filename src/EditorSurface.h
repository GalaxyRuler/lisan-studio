#pragma once

#include "ApyHighlighter.h"
#include "DocumentFileIO.h"
#include "EditorFindService.h"

#include <QContextMenuEvent>
#include <QInputMethodEvent>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QStringList>
#include <QTimer>
#include <QVector>

class LineNumberArea;

struct HiddenBidiFinding
{
    int position = 0;
    QString character;
    QString unicodeName;
};

struct EditorCompletionItem
{
    QString label;
    QString detail;
    QString insertText;
};

class EditorSurface final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit EditorSurface(QWidget *parent = nullptr);

    bool openFile(const QString &path, QString *error = nullptr);
    void resetForNewFile();
    bool saveFile(QString *error = nullptr);
    bool saveFileAs(const QString &path, QString *error = nullptr);

    QString currentFilePath() const;
    bool isDirty() const;
    QVector<HiddenBidiFinding> findHiddenBidiControls(const QString &text) const;
    int setFindQuery(const QString &query);
    QString findQuery() const;
    int findMatchCount() const;
    int currentFindMatchIndex() const;
    bool selectNextFindMatch();
    bool selectPreviousFindMatch();
    bool replaceCurrentFindMatch(const QString &replacement);
    int replaceAllFindMatches(const QString &replacement);
    int findHighlightSelectionCountForTest() const;
    int bracketMatchSelectionCountForTest() const;
    bool isVisibleWhitespaceEnabled() const;
    void setVisibleWhitespaceEnabled(bool enabled);
    bool trimTrailingWhitespaceOnSave() const;
    void setTrimTrailingWhitespaceOnSave(bool enabled);
    bool indentationGuidesEnabled() const;
    void setIndentationGuidesEnabled(bool enabled);
    int indentationGuideCountForLineForTest(const QString &line) const;
    static constexpr int kSoftCursorCap = 100;
    static constexpr int kHardCursorCap = 1000;
    int totalCursorCount() const;
    QVector<QTextCursor> secondaryCursorsForTest() const;
    bool addCursorAtPosition(int position);
    bool addCursorAbovePrimary();
    bool addCursorBelowPrimary();
    bool addCursorAtNextMatch();
    int selectAllFindMatchesAsCursors();
    int generateColumnSelectionBetween(const QTextCursor &anchor, const QTextCursor &release);
    void collapseToSinglePrimaryCursor();
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    QMenu *createEditorContextMenu(QWidget *parent = nullptr);
    void showCompletionItems(const QVector<EditorCompletionItem> &items);
    void showHoverMarkdown(const QString &markdown, const QPoint &viewportPosition);
    bool isCompletionPopupVisibleForTest() const;
    QStringList completionLabelsForTest() const;
    QString visibleHoverTextForTest() const;

signals:
    void filePathChanged(const QString &path);
    void dirtyStateChanged(bool dirty);
    void cursorSoftCapReached(int totalCursors);
    void cursorCountChanged(int totalCursors);
    void completionRequested(int line, int character);
    void hoverRequested(int line, int character, QPoint viewportPosition);
    void definitionRequested(int line, int character);

private:
    QString filePath;
    QString emptyPlaceholderText;
    QString activeFindQuery;
    QVector<EditorFindMatch> activeFindMatches;
    int activeFindIndex = -1;
    int findHighlightSelectionCount = 0;
    int bracketMatchSelectionCount = 0;
    bool trimTrailingWhitespace = false;
    bool showIndentationGuides = true;
    bool cursorSoftCapNotified = false;
    bool inAltColumnDrag = false;
    DocumentLineEnding saveLineEnding = DocumentLineEnding::None;
    ApyHighlighter *highlighter = nullptr;
    LineNumberArea *lineNumberArea = nullptr;
    QListWidget *completionPopup = nullptr;
    QTimer completionRequestTimer;
    QTimer hoverRequestTimer;
    QVector<QTextCursor> secondaryCursors;
    QTextCursor altColumnDragAnchor;
    QPoint pendingHoverViewportPosition;
    QString lastHoverMarkdown;

    static QString unicodeName(QChar ch);
    void setCurrentFilePath(const QString &path);
    void updateDocumentDirectionPolicy();
    void refreshFindMatches(bool selectFirst = true);
    void updateEditorExtraSelections();
    QVector<int> matchingDelimiterPositions() const;
    void paintIndentationGuides(QPainter *painter);
    void paintSecondaryCarets(QPainter *painter);
    QVector<QTextCursor> allCursorsInDocumentOrderDescending() const;
    bool isPositionAlreadyCovered(int position) const;
    bool selectFindMatch(int index);
    void updateLineNumberAreaWidth(int blockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void requestCompletionAtPrimaryCursor();
    void scheduleCompletionRequest();
    void requestHoverAtViewportPosition(const QPoint &position);
    void insertSelectedCompletion();
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
