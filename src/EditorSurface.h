#pragma once

#include "ApyHighlighter.h"
#include "DocumentFileIO.h"
#include "EditorFindService.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QPlainTextEdit>
#include <QVector>

class LineNumberArea;

struct HiddenBidiFinding
{
    int position = 0;
    QString character;
    QString unicodeName;
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
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    QMenu *createEditorContextMenu(QWidget *parent = nullptr);

signals:
    void filePathChanged(const QString &path);
    void dirtyStateChanged(bool dirty);

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
    DocumentLineEnding saveLineEnding = DocumentLineEnding::Lf;
    ApyHighlighter *highlighter = nullptr;
    LineNumberArea *lineNumberArea = nullptr;

    static QString unicodeName(QChar ch);
    void setCurrentFilePath(const QString &path);
    void refreshFindMatches(bool selectFirst = true);
    void updateEditorExtraSelections();
    QVector<int> matchingDelimiterPositions() const;
    void paintIndentationGuides(QPainter *painter);
    bool selectFindMatch(int index);
    void updateLineNumberAreaWidth(int blockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
