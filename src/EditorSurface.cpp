#include "EditorSurface.h"

#include "DocumentFileIO.h"

#include <QAbstractItemView>
#include <QPainter>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPaintEvent>
#include <QTextBlock>
#include <QTextOption>
#include <QToolTip>

#include <algorithm>
#include <memory>

static bool isQuoteDelimiter(QChar ch)
{
    return ch == QLatin1Char('"') || ch == QLatin1Char('\'');
}

static QChar closingDelimiterFor(QChar ch)
{
    switch (ch.unicode()) {
    case '(':
        return QLatin1Char(')');
    case '[':
        return QLatin1Char(']');
    case '{':
        return QLatin1Char('}');
    case 0x00AB:
        return QChar(0x00BB);
    default:
        return QChar();
    }
}

static QChar openingDelimiterFor(QChar ch)
{
    switch (ch.unicode()) {
    case ')':
        return QLatin1Char('(');
    case ']':
        return QLatin1Char('[');
    case '}':
        return QLatin1Char('{');
    case 0x00BB:
        return QChar(0x00AB);
    default:
        return QChar();
    }
}

static QString removeTrailingWhitespace(const QString &text)
{
    QString result;
    result.reserve(text.size());

    int lineStart = 0;
    for (int i = 0; i <= text.size(); ++i) {
        if (i < text.size() && text.at(i) != QLatin1Char('\n')) {
            continue;
        }

        int lineEnd = i;
        while (lineEnd > lineStart) {
            const QChar ch = text.at(lineEnd - 1);
            if (ch != QLatin1Char(' ') && ch != QLatin1Char('\t')) {
                break;
            }
            --lineEnd;
        }

        result.append(text.mid(lineStart, lineEnd - lineStart));
        if (i < text.size()) {
            result.append(QLatin1Char('\n'));
        }
        lineStart = i + 1;
    }

    return result;
}

static QString withSavedLineEnding(QString text, DocumentLineEnding lineEnding)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    if (lineEnding != DocumentLineEnding::Crlf) {
        return text;
    }

    text.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
    return text;
}

static DocumentLineEnding dominantLineEndingForMixedText(const QString &text)
{
    int crlfCount = 0;
    int lfCount = 0;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) != QLatin1Char('\n')) {
            continue;
        }

        if (i > 0 && text.at(i - 1) == QLatin1Char('\r')) {
            ++crlfCount;
        } else {
            ++lfCount;
        }
    }

    return crlfCount >= lfCount ? DocumentLineEnding::Crlf : DocumentLineEnding::Lf;
}

static DocumentLineEnding saveLineEndingForLoadedText(const DocumentLoadResult &loaded)
{
    if (loaded.lineEnding == DocumentLineEnding::Mixed) {
        return dominantLineEndingForMixedText(loaded.text);
    }
    return loaded.lineEnding;
}

static DocumentLineEnding resolveSaveLineEnding(DocumentLineEnding lineEnding)
{
    // New/empty documents default to Windows CRLF; mixed files are resolved to their dominant style on load.
    if (lineEnding == DocumentLineEnding::None || lineEnding == DocumentLineEnding::Mixed) {
        return DocumentLineEnding::Crlf;
    }
    return lineEnding;
}

static bool containsStrongRtlText(const QString &text)
{
    for (QChar ch : text) {
        const QChar::Direction direction = ch.direction();
        if (direction == QChar::DirR || direction == QChar::DirAL) {
            return true;
        }
    }
    return false;
}

static bool isPlainPrintableKey(const QKeyEvent *event)
{
    const Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool printableModifiers = modifiers == Qt::NoModifier
        || modifiers == Qt::KeypadModifier
        || modifiers == Qt::ShiftModifier
        || modifiers == (Qt::ShiftModifier | Qt::KeypadModifier);
    return printableModifiers
        && !event->text().isEmpty()
        && event->text().at(0).category() != QChar::Other_Control
        && event->key() != Qt::Key_Return
        && event->key() != Qt::Key_Enter;
}

static bool isIdentifierCharacter(QChar ch)
{
    return ch == QLatin1Char('_') || ch.isLetterOrNumber();
}

static QString returnIndentForCursor(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    const QString line = block.text();
    const int cursorColumn = qBound(0, cursor.position() - block.position(), line.size());
    const QString beforeCursor = line.left(cursorColumn);

    QString indent;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t')) {
            indent.append(ch);
            continue;
        }
        break;
    }
    if (beforeCursor.trimmed().endsWith(QLatin1Char(':'))) {
        indent.append(QStringLiteral("    "));
    }
    return indent;
}

static QColor semanticTokenColor(const QString &tokenType)
{
    if (tokenType == QStringLiteral("function") || tokenType == QStringLiteral("method")) {
        return QColor(QStringLiteral("#7BDFF2"));
    }
    if (tokenType == QStringLiteral("class") || tokenType == QStringLiteral("type") || tokenType == QStringLiteral("interface")) {
        return QColor(QStringLiteral("#B8E986"));
    }
    if (tokenType == QStringLiteral("keyword")) {
        return QColor(QStringLiteral("#F6D365"));
    }
    if (tokenType == QStringLiteral("string")) {
        return QColor(QStringLiteral("#F5A97F"));
    }
    if (tokenType == QStringLiteral("number")) {
        return QColor(QStringLiteral("#C792EA"));
    }
    return QColor(QStringLiteral("#AEC6FF"));
}

static void indentBlockByOneLevel(QTextDocument *document, int blockNumber)
{
    const QTextBlock block = document->findBlockByNumber(blockNumber);
    if (!block.isValid()) {
        return;
    }

    QTextCursor cursor(block);
    cursor.setPosition(block.position());
    cursor.insertText(QStringLiteral("    "));
}

static void dedentBlockByOneLevel(QTextDocument *document, int blockNumber)
{
    const QTextBlock block = document->findBlockByNumber(blockNumber);
    if (!block.isValid()) {
        return;
    }

    const QString line = block.text();
    int spacesToRemove = 0;
    while (spacesToRemove < 4
        && spacesToRemove < line.size()
        && line.at(spacesToRemove) == QLatin1Char(' ')) {
        ++spacesToRemove;
    }

    QTextCursor cursor(block);
    cursor.setPosition(block.position());
    if (spacesToRemove > 0) {
        cursor.setPosition(block.position() + spacesToRemove, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        return;
    }

    if (!line.isEmpty() && line.at(0) == QLatin1Char('\t')) {
        cursor.setPosition(block.position() + 1, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
}

class LineNumberArea final : public QWidget
{
public:
    explicit LineNumberArea(EditorSurface *editor)
        : QWidget(editor),
          editorSurface(editor)
    {
        setObjectName(QStringLiteral("lineNumberArea"));
        setLayoutDirection(Qt::LeftToRight);
    }

    QSize sizeHint() const override
    {
        return QSize(editorSurface->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        editorSurface->lineNumberAreaPaintEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        editorSurface->lineNumberAreaMousePressEvent(event);
    }

private:
    EditorSurface *editorSurface = nullptr;
};

EditorSurface::EditorSurface(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setLayoutDirection(Qt::RightToLeft);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setUndoRedoEnabled(true);
    setMouseTracking(true);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    emptyPlaceholderText = QString::fromUtf8("اكتب كود لغة الثعبان هنا");
    setPlaceholderText(QString());

    QTextOption option = document()->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
    document()->setDefaultTextOption(option);

    QFont font(QStringLiteral("Cascadia Code"));
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(12);
    setFont(font);

    lineNumberArea = new LineNumberArea(this);
    updateLineNumberAreaWidth(blockCount());

    completionPopup = new QListWidget(viewport());
    completionPopup->setObjectName(QStringLiteral("lspCompletionPopup"));
    completionPopup->setFocusPolicy(Qt::NoFocus);
    completionPopup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    completionPopup->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    completionPopup->hide();

    completionRequestTimer.setSingleShot(true);
    completionRequestTimer.setInterval(120);
    connect(&completionRequestTimer, &QTimer::timeout, this, &EditorSurface::requestCompletionAtPrimaryCursor);

    hoverRequestTimer.setSingleShot(true);
    hoverRequestTimer.setInterval(250);
    connect(&hoverRequestTimer, &QTimer::timeout, this, [this]() {
        requestHoverAtViewportPosition(pendingHoverViewportPosition);
    });

    highlighter = new ApyHighlighter(document());

    connect(document(), &QTextDocument::modificationChanged, this, &EditorSurface::dirtyStateChanged);
    connect(document(), &QTextDocument::contentsChanged, this, [this]() {
        updateDocumentDirectionPolicy();
        if (!activeFindQuery.isEmpty()) {
            refreshFindMatches(false);
        } else {
            updateEditorExtraSelections();
        }
    });
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &EditorSurface::updateEditorExtraSelections);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &EditorSurface::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &EditorSurface::updateLineNumberArea);
}

bool EditorSurface::openFile(const QString &path, QString *error)
{
    const DocumentLoadResult loaded = DocumentFileIO::loadUtf8(path, error);
    if (loaded.text.isEmpty() && error && !error->isEmpty()) {
        return false;
    }

    setPlainText(loaded.text);
    document()->setModified(false);
    collapseToSinglePrimaryCursor();
    saveLineEnding = saveLineEndingForLoadedText(loaded);
    setCurrentFilePath(loaded.identity.path);
    return true;
}

void EditorSurface::resetForNewFile()
{
    clear();
    document()->setModified(false);
    collapseToSinglePrimaryCursor();
    saveLineEnding = DocumentLineEnding::None;
    setCurrentFilePath(QString());
    updateDocumentDirectionPolicy();
}

void EditorSurface::updateDocumentDirectionPolicy()
{
    QTextOption option = document()->defaultTextOption();
    const QString text = toPlainText();
    const bool useRtlDocument = text.isEmpty() || containsStrongRtlText(text);
    option.setTextDirection(useRtlDocument ? Qt::RightToLeft : Qt::LeftToRight);
    option.setAlignment(useRtlDocument ? Qt::AlignRight : Qt::AlignLeft);
    document()->setDefaultTextOption(option);
    viewport()->update();
}

bool EditorSurface::saveFile(QString *error)
{
    if (filePath.isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("لا يوجد مسار محفوظ لهذا الملف.");
        }
        return false;
    }
    return saveFileAs(filePath, error);
}

bool EditorSurface::saveFileAs(const QString &path, QString *error)
{
    QString textToSave = toPlainText();
    if (trimTrailingWhitespace) {
        textToSave = removeTrailingWhitespace(textToSave);
        if (textToSave != toPlainText()) {
            QTextCursor cursor(document());
            cursor.select(QTextCursor::Document);
            cursor.insertText(textToSave);
        }
    }

    const QString savedText = withSavedLineEnding(textToSave, resolveSaveLineEnding(saveLineEnding));
    if (!DocumentFileIO::saveUtf8Atomically(path, savedText, error)) {
        return false;
    }

    document()->setModified(false);
    saveLineEnding = DocumentFileIO::detectLineEnding(savedText);
    setCurrentFilePath(DocumentFileIO::identityForPath(path).path);
    return true;
}

QString EditorSurface::currentFilePath() const
{
    return filePath;
}

bool EditorSurface::isDirty() const
{
    return document()->isModified();
}

QVector<HiddenBidiFinding> EditorSurface::findHiddenBidiControls(const QString &text) const
{
    QVector<HiddenBidiFinding> findings;
    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ApyHighlighter::isHiddenBidiControl(ch)) {
            findings.push_back({i, QString(ch), unicodeName(ch)});
        }
    }
    return findings;
}

int EditorSurface::setFindQuery(const QString &query)
{
    activeFindQuery = query;
    refreshFindMatches(true);
    return activeFindMatches.size();
}

QString EditorSurface::findQuery() const
{
    return activeFindQuery;
}

int EditorSurface::findMatchCount() const
{
    return activeFindMatches.size();
}

int EditorSurface::currentFindMatchIndex() const
{
    return activeFindIndex;
}

bool EditorSurface::selectNextFindMatch()
{
    if (activeFindMatches.isEmpty()) {
        return false;
    }

    const int nextIndex = activeFindIndex < 0
        ? 0
        : (activeFindIndex + 1) % activeFindMatches.size();
    return selectFindMatch(nextIndex);
}

bool EditorSurface::selectPreviousFindMatch()
{
    if (activeFindMatches.isEmpty()) {
        return false;
    }

    const int previousIndex = activeFindIndex <= 0
        ? activeFindMatches.size() - 1
        : activeFindIndex - 1;
    return selectFindMatch(previousIndex);
}

bool EditorSurface::replaceCurrentFindMatch(const QString &replacement)
{
    if (activeFindIndex < 0 || activeFindIndex >= activeFindMatches.size()) {
        return false;
    }

    const EditorFindMatch match = activeFindMatches.at(activeFindIndex);
    QTextCursor cursor(document());
    cursor.setPosition(match.start);
    cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);
    cursor.insertText(replacement);
    refreshFindMatches(true);
    return true;
}

int EditorSurface::replaceAllFindMatches(const QString &replacement)
{
    if (activeFindQuery.isEmpty()) {
        return 0;
    }

    int replaced = 0;
    const QString replacedText = EditorFindService::replaceAll(toPlainText(), activeFindQuery, replacement, &replaced);
    if (replaced <= 0) {
        return 0;
    }

    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);
    cursor.insertText(replacedText);
    refreshFindMatches(true);
    return replaced;
}

int EditorSurface::findHighlightSelectionCountForTest() const
{
    return findHighlightSelectionCount;
}

int EditorSurface::bracketMatchSelectionCountForTest() const
{
    return bracketMatchSelectionCount;
}

bool EditorSurface::isVisibleWhitespaceEnabled() const
{
    return document()->defaultTextOption().flags() & QTextOption::ShowTabsAndSpaces;
}

void EditorSurface::setVisibleWhitespaceEnabled(bool enabled)
{
    QTextOption option = document()->defaultTextOption();
    QTextOption::Flags flags = option.flags();
    if (enabled) {
        flags |= QTextOption::ShowTabsAndSpaces;
    } else {
        flags &= ~QTextOption::ShowTabsAndSpaces;
    }
    option.setFlags(flags);
    document()->setDefaultTextOption(option);
    viewport()->update();
}

bool EditorSurface::trimTrailingWhitespaceOnSave() const
{
    return trimTrailingWhitespace;
}

void EditorSurface::setTrimTrailingWhitespaceOnSave(bool enabled)
{
    trimTrailingWhitespace = enabled;
}

bool EditorSurface::indentationGuidesEnabled() const
{
    return showIndentationGuides;
}

void EditorSurface::setIndentationGuidesEnabled(bool enabled)
{
    if (showIndentationGuides == enabled) {
        return;
    }
    showIndentationGuides = enabled;
    viewport()->update();
}

int EditorSurface::indentationGuideCountForLineForTest(const QString &line) const
{
    int columns = 0;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' ')) {
            ++columns;
        } else if (ch == QLatin1Char('\t')) {
            columns += 4;
        } else {
            break;
        }
    }
    return columns / 4;
}

int EditorSurface::totalCursorCount() const
{
    return 1 + secondaryCursors.size();
}

QVector<QTextCursor> EditorSurface::secondaryCursorsForTest() const
{
    return secondaryCursors;
}

bool EditorSurface::isPositionAlreadyCovered(int position) const
{
    if (textCursor().position() == position) {
        return true;
    }
    for (const QTextCursor &cursor : secondaryCursors) {
        if (cursor.position() == position) {
            return true;
        }
    }
    return false;
}

bool EditorSurface::addCursorAtPosition(int position)
{
    if (totalCursorCount() >= kHardCursorCap) {
        return false;
    }
    position = qBound(0, position, document()->characterCount() - 1);
    if (isPositionAlreadyCovered(position)) {
        return false;
    }

    const int beforeCount = totalCursorCount();
    QTextCursor cursor(document());
    cursor.setPosition(position);
    secondaryCursors.append(cursor);
    const int afterCount = totalCursorCount();
    if (beforeCount < kSoftCursorCap && afterCount >= kSoftCursorCap && !cursorSoftCapNotified) {
        cursorSoftCapNotified = true;
        emit cursorSoftCapReached(afterCount);
    }
    emit cursorCountChanged(afterCount);
    viewport()->update();
    return true;
}

bool EditorSurface::addCursorAbovePrimary()
{
    const QTextCursor primary = textCursor();
    const QTextBlock block = primary.block();
    const QTextBlock previous = block.previous();
    if (!previous.isValid()) {
        return false;
    }

    const int column = primary.position() - block.position();
    const int targetColumn = qMin(column, previous.length() - 1);
    return addCursorAtPosition(previous.position() + qMax(0, targetColumn));
}

bool EditorSurface::addCursorBelowPrimary()
{
    const QTextCursor primary = textCursor();
    const QTextBlock block = primary.block();
    const QTextBlock next = block.next();
    if (!next.isValid()) {
        return false;
    }

    const int column = primary.position() - block.position();
    const int targetColumn = qMin(column, next.length() - 1);
    return addCursorAtPosition(next.position() + qMax(0, targetColumn));
}

bool EditorSurface::addCursorAtNextMatch()
{
    const QTextCursor primary = textCursor();
    if (!primary.hasSelection()) {
        return false;
    }

    QString selectedText = primary.selectedText();
    selectedText.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    if (selectedText.isEmpty()) {
        return false;
    }

    const QString text = toPlainText();
    const int selectionStart = primary.selectionStart();
    const int selectionEnd = primary.selectionEnd();
    int searchStart = selectionEnd;
    bool wrapped = false;
    while (true) {
        const int matchStart = text.indexOf(selectedText, searchStart, Qt::CaseSensitive);
        if (matchStart >= 0 && matchStart != selectionStart && !isPositionAlreadyCovered(matchStart)) {
            if (!addCursorAtPosition(matchStart)) {
                return false;
            }
            QTextCursor &secondary = secondaryCursors.last();
            secondary.setPosition(matchStart + selectedText.size());
            secondary.setPosition(matchStart, QTextCursor::KeepAnchor);
            viewport()->update();
            return true;
        }

        if (matchStart >= 0) {
            searchStart = matchStart + qMax(1, selectedText.size());
            continue;
        }

        if (wrapped) {
            return false;
        }
        wrapped = true;
        searchStart = 0;
    }
}

int EditorSurface::selectAllFindMatchesAsCursors()
{
    if (activeFindQuery.isEmpty() || activeFindMatches.isEmpty()) {
        return 0;
    }

    int added = 0;
    const QTextCursor primary = textCursor();
    for (const EditorFindMatch &match : activeFindMatches) {
        if (totalCursorCount() >= kHardCursorCap) {
            break;
        }
        const int start = match.start;
        const int end = match.start + match.length;
        const bool primaryCoversMatch = primary.hasSelection()
            && primary.selectionStart() == start
            && primary.selectionEnd() == end;
        if (primaryCoversMatch) {
            continue;
        }
        if (!addCursorAtPosition(start)) {
            continue;
        }

        QTextCursor &secondary = secondaryCursors.last();
        secondary.setPosition(end);
        secondary.setPosition(start, QTextCursor::KeepAnchor);
        ++added;
    }
    viewport()->update();
    return added;
}

int EditorSurface::generateColumnSelectionBetween(const QTextCursor &anchor, const QTextCursor &release)
{
    if (totalCursorCount() >= kHardCursorCap) {
        return 0;
    }

    const int anchorBlock = anchor.blockNumber();
    const int releaseBlock = release.blockNumber();
    const int anchorCol = qMax(0, anchor.position() - anchor.block().position());
    const int releaseCol = qMax(0, release.position() - release.block().position());
    const int startBlock = qMin(anchorBlock, releaseBlock);
    const int endBlock = qMax(anchorBlock, releaseBlock);
    const int leftCol = qMin(anchorCol, releaseCol);
    const int rightCol = qMax(anchorCol, releaseCol);

    int added = 0;
    for (int blockIndex = startBlock; blockIndex <= endBlock; ++blockIndex) {
        if (totalCursorCount() >= kHardCursorCap) {
            break;
        }

        const QTextBlock block = document()->findBlockByNumber(blockIndex);
        if (!block.isValid()) {
            continue;
        }

        const int lineLen = qMax(0, block.length() - 1);
        const int clampedLeft = qMin(leftCol, lineLen);
        const int clampedRight = qMin(rightCol, lineLen);
        const int leftPosition = block.position() + clampedLeft;
        const int rightPosition = block.position() + clampedRight;
        const bool usePrimary = blockIndex == startBlock && secondaryCursors.isEmpty();

        QTextCursor columnCursor(document());
        columnCursor.setPosition(leftPosition);
        if (clampedLeft != clampedRight) {
            columnCursor.setPosition(rightPosition, QTextCursor::KeepAnchor);
        }

        if (usePrimary) {
            setTextCursor(columnCursor);
            continue;
        }

        if (!addCursorAtPosition(clampedLeft == clampedRight ? leftPosition : rightPosition)) {
            continue;
        }
        QTextCursor &secondary = secondaryCursors.last();
        secondary.setPosition(leftPosition);
        if (clampedLeft != clampedRight) {
            secondary.setPosition(rightPosition, QTextCursor::KeepAnchor);
        }
        ++added;
    }

    viewport()->update();
    return added;
}

void EditorSurface::collapseToSinglePrimaryCursor()
{
    if (secondaryCursors.isEmpty()) {
        return;
    }
    secondaryCursors.clear();
    cursorSoftCapNotified = false;
    emit cursorCountChanged(totalCursorCount());
    viewport()->update();
}

QVector<QTextCursor> EditorSurface::allCursorsInDocumentOrderDescending() const
{
    QVector<QTextCursor> cursors = secondaryCursors;
    cursors.append(textCursor());
    std::sort(cursors.begin(), cursors.end(), [](const QTextCursor &left, const QTextCursor &right) {
        return left.position() > right.position();
    });
    return cursors;
}

void EditorSurface::paintSecondaryCarets(QPainter *painter)
{
    if (!painter) {
        return;
    }

    QColor selectionFill(QStringLiteral("#3A2F12"));
    selectionFill.setAlpha(160);
    painter->save();
    for (const QTextCursor &cursor : secondaryCursors) {
        if (cursor.hasSelection()) {
            const int start = cursor.selectionStart();
            const int end = cursor.selectionEnd();
            QTextCursor lineCursor(document());
            lineCursor.setPosition(start);
            while (lineCursor.position() < end) {
                const int lineStart = lineCursor.position();
                const QTextBlock block = lineCursor.block();
                const int lineEnd = qMin(end, block.position() + block.length() - 1);

                QTextCursor startCursor(document());
                startCursor.setPosition(lineStart);
                QTextCursor endCursor(document());
                endCursor.setPosition(lineEnd);
                QRect highlightRect = cursorRect(startCursor);
                const QRect endRect = cursorRect(endCursor);
                highlightRect.setRight(qMax(highlightRect.right() + fontMetrics().horizontalAdvance(QLatin1Char(' ')), endRect.left()));
                highlightRect.setHeight(fontMetrics().height());
                painter->fillRect(highlightRect, selectionFill);

                if (lineEnd >= end) {
                    break;
                }
                lineCursor.setPosition(lineEnd + 1);
            }
        }

        const QRect caretRect = cursorRect(cursor);
        painter->setPen(palette().text().color());
        painter->drawLine(caretRect.topLeft(), caretRect.bottomLeft());
    }
    painter->restore();
}

int EditorSurface::lineNumberAreaWidth() const
{
    int digits = 1;
    int maximum = qMax(1, blockCount());
    while (maximum >= 10) {
        maximum /= 10;
        ++digits;
    }

    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

bool EditorSurface::hasBreakpointAtLine(int line) const
{
    return breakpointLines.contains(line);
}

bool EditorSurface::setBreakpointAtLine(int line, bool enabled)
{
    if (line < 1 || line > blockCount()) {
        return false;
    }
    const bool currentlyEnabled = breakpointLines.contains(line);
    if (currentlyEnabled == enabled) {
        return false;
    }
    if (enabled) {
        breakpointLines.insert(line);
    } else {
        breakpointLines.remove(line);
    }
    if (lineNumberArea) {
        lineNumberArea->update();
    }
    emit breakpointToggled(line, enabled);
    return true;
}

bool EditorSurface::toggleBreakpointAtLine(int line)
{
    return setBreakpointAtLine(line, !hasBreakpointAtLine(line));
}

QVector<int> EditorSurface::breakpointLinesForTest() const
{
    QVector<int> lines = breakpointLines.values().toVector();
    std::sort(lines.begin(), lines.end());
    return lines;
}

QMenu *EditorSurface::createEditorContextMenu(QWidget *parent)
{
    auto *menu = new QMenu(parent ? parent : this);
    menu->setLayoutDirection(Qt::RightToLeft);

    auto *undoAction = menu->addAction(QString::fromUtf8("تراجع"));
    undoAction->setObjectName(QStringLiteral("editorContextUndoAction"));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(document()->isUndoAvailable());
    connect(undoAction, &QAction::triggered, this, &QPlainTextEdit::undo);

    auto *redoAction = menu->addAction(QString::fromUtf8("إعادة"));
    redoAction->setObjectName(QStringLiteral("editorContextRedoAction"));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(document()->isRedoAvailable());
    connect(redoAction, &QAction::triggered, this, &QPlainTextEdit::redo);

    menu->addSeparator();

    auto *cutAction = menu->addAction(QString::fromUtf8("قص"));
    cutAction->setObjectName(QStringLiteral("editorContextCutAction"));
    cutAction->setShortcut(QKeySequence::Cut);
    cutAction->setEnabled(textCursor().hasSelection());
    connect(cutAction, &QAction::triggered, this, &QPlainTextEdit::cut);

    auto *copyAction = menu->addAction(QString::fromUtf8("نسخ"));
    copyAction->setObjectName(QStringLiteral("editorContextCopyAction"));
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(textCursor().hasSelection());
    connect(copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);

    auto *pasteAction = menu->addAction(QString::fromUtf8("لصق"));
    pasteAction->setObjectName(QStringLiteral("editorContextPasteAction"));
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setEnabled(canPaste());
    connect(pasteAction, &QAction::triggered, this, &QPlainTextEdit::paste);

    menu->addSeparator();

    auto *selectAllAction = menu->addAction(QString::fromUtf8("تحديد الكل"));
    selectAllAction->setObjectName(QStringLiteral("editorContextSelectAllAction"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    selectAllAction->setEnabled(!document()->isEmpty());
    connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);

    return menu;
}

void EditorSurface::showCompletionItems(const QVector<EditorCompletionItem> &items)
{
    completionPopup->clear();
    if (items.isEmpty()) {
        completionPopup->hide();
        return;
    }

    for (const EditorCompletionItem &item : items) {
        auto *listItem = new QListWidgetItem(item.detail.isEmpty()
                ? item.label
                : QStringLiteral("%1  %2").arg(item.label, item.detail),
            completionPopup);
        listItem->setData(Qt::UserRole, item.insertText.isEmpty() ? item.label : item.insertText);
        listItem->setData(Qt::UserRole + 1, item.label);
    }

    const QRect caret = cursorRect();
    const int width = qMin(360, qMax(180, viewport()->width() - 12));
    const int rowHeight = qMax(completionPopup->sizeHintForRow(0), fontMetrics().height() + 8);
    const int height = qMin(rowHeight * qMin(items.size(), 6) + 4, qMax(64, viewport()->height() / 2));
    const int left = qBound(4, caret.left(), qMax(4, viewport()->width() - width - 4));
    int top = caret.bottom() + 4;
    if (top + height > viewport()->height()) {
        top = qMax(4, caret.top() - height - 4);
    }
    completionPopup->setGeometry(left, top, width, height);
    completionPopup->setCurrentRow(0);
    completionPopup->show();
    completionPopup->raise();
}

void EditorSurface::showHoverMarkdown(const QString &markdown, const QPoint &viewportPosition)
{
    lastHoverMarkdown = markdown;
    if (markdown.trimmed().isEmpty()) {
        QToolTip::hideText();
        return;
    }
    QToolTip::showText(viewport()->mapToGlobal(viewportPosition), markdown, viewport());
}

void EditorSurface::setSemanticTokens(const QVector<EditorSemanticToken> &tokens)
{
    semanticTokens = tokens;
    updateEditorExtraSelections();
}

bool EditorSurface::isCompletionPopupVisibleForTest() const
{
    return completionPopup && completionPopup->isVisible();
}

QStringList EditorSurface::completionLabelsForTest() const
{
    QStringList labels;
    if (!completionPopup) {
        return labels;
    }
    for (int i = 0; i < completionPopup->count(); ++i) {
        labels.append(completionPopup->item(i)->data(Qt::UserRole + 1).toString());
    }
    return labels;
}

QString EditorSurface::visibleHoverTextForTest() const
{
    return lastHoverMarkdown;
}

int EditorSurface::semanticTokenSelectionCountForTest() const
{
    return semanticTokenSelectionCount;
}

void EditorSurface::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(33, 37, 41));
    painter.setPen(QColor(173, 181, 189));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            if (breakpointLines.contains(blockNumber + 1)) {
                const int diameter = qMin(10, qMax(6, fontMetrics().height() - 4));
                const QRect markerRect(4, top + (fontMetrics().height() - diameter) / 2, diameter, diameter);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(QStringLiteral("#D84F4F")));
                painter.drawEllipse(markerRect);
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QColor(173, 181, 189));
            }
            painter.drawText(
                0,
                top,
                lineNumberArea->width() - 6,
                fontMetrics().height(),
                Qt::AlignRight,
                number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void EditorSurface::lineNumberAreaMousePressEvent(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        if (event) {
            event->ignore();
        }
        return;
    }
    const int line = lineNumberForViewportY(event->pos().y());
    if (line >= 1) {
        toggleBreakpointAtLine(line);
        event->accept();
        return;
    }
    event->ignore();
}

int EditorSurface::lineNumberForViewportY(int y) const
{
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= viewport()->rect().bottom()) {
        if (block.isVisible() && y >= top && y <= bottom) {
            return block.blockNumber() + 1;
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
    return -1;
}

QString EditorSurface::unicodeName(QChar ch)
{
    switch (ch.unicode()) {
    case 0x202A:
        return QStringLiteral("LEFT-TO-RIGHT EMBEDDING");
    case 0x202B:
        return QStringLiteral("RIGHT-TO-LEFT EMBEDDING");
    case 0x202C:
        return QStringLiteral("POP DIRECTIONAL FORMATTING");
    case 0x202D:
        return QStringLiteral("LEFT-TO-RIGHT OVERRIDE");
    case 0x202E:
        return QStringLiteral("RIGHT-TO-LEFT OVERRIDE");
    case 0x2066:
        return QStringLiteral("LEFT-TO-RIGHT ISOLATE");
    case 0x2067:
        return QStringLiteral("RIGHT-TO-LEFT ISOLATE");
    case 0x2068:
        return QStringLiteral("FIRST STRONG ISOLATE");
    case 0x2069:
        return QStringLiteral("POP DIRECTIONAL ISOLATE");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

void EditorSurface::setCurrentFilePath(const QString &path)
{
    if (filePath == path) {
        return;
    }
    filePath = path;
    emit filePathChanged(filePath);
}

void EditorSurface::refreshFindMatches(bool selectFirst)
{
    activeFindMatches = EditorFindService::findAll(toPlainText(), activeFindQuery);
    if (activeFindMatches.isEmpty()) {
        activeFindIndex = -1;
        updateEditorExtraSelections();
        return;
    }

    if (selectFirst || activeFindIndex < 0 || activeFindIndex >= activeFindMatches.size()) {
        selectFindMatch(0);
        return;
    }

    updateEditorExtraSelections();
}

void EditorSurface::updateEditorExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;
    for (int i = 0; i < activeFindMatches.size(); ++i) {
        const EditorFindMatch match = activeFindMatches.at(i);
        QTextCursor cursor(document());
        cursor.setPosition(match.start);
        cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format.setBackground(i == activeFindIndex
            ? QColor(QStringLiteral("#4C8DFF"))
            : QColor(QStringLiteral("#3A2F12")));
        selection.format.setForeground(QColor(QStringLiteral("#FFFFFF")));
        selections.push_back(selection);
    }

    findHighlightSelectionCount = selections.size();
    bracketMatchSelectionCount = 0;
    semanticTokenSelectionCount = 0;
    const QVector<int> delimiterPositions = matchingDelimiterPositions();
    for (const int position : delimiterPositions) {
        QTextCursor cursor(document());
        cursor.setPosition(position);
        cursor.setPosition(position + 1, QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format.setBackground(QColor(QStringLiteral("#5B3C88")));
        selection.format.setForeground(QColor(QStringLiteral("#FFFFFF")));
        selections.push_back(selection);
        ++bracketMatchSelectionCount;
    }

    for (const EditorSemanticToken &token : semanticTokens) {
        const QTextBlock block = document()->findBlockByNumber(token.line);
        if (!block.isValid() || token.startCharacter < 0 || token.length <= 0) {
            continue;
        }
        const QString line = block.text();
        if (token.startCharacter >= line.size()) {
            continue;
        }

        QTextCursor cursor(block);
        const int start = block.position() + token.startCharacter;
        const int end = qMin(start + token.length, block.position() + line.size());
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format.setForeground(semanticTokenColor(token.tokenType));
        if (token.tokenType == QStringLiteral("function") || token.tokenType == QStringLiteral("method")) {
            selection.format.setFontWeight(QFont::DemiBold);
        }
        selections.push_back(selection);
        ++semanticTokenSelectionCount;
    }

    setExtraSelections(selections);
}

QVector<int> EditorSurface::matchingDelimiterPositions() const
{
    const QString text = toPlainText();
    if (text.isEmpty()) {
        return {};
    }

    auto matchingQuoteAt = [&](int position) -> QVector<int> {
        const QChar quote = text.at(position);
        const QTextBlock block = document()->findBlock(position);
        const int blockStart = block.position();
        const int blockEnd = blockStart + block.text().size();

        for (int i = position + 1; i < blockEnd; ++i) {
            if (text.at(i) == quote) {
                return {position, i};
            }
        }
        for (int i = position - 1; i >= blockStart; --i) {
            if (text.at(i) == quote) {
                return {i, position};
            }
        }
        return {};
    };

    auto matchingOpenAt = [&](int position, QChar open, QChar close) -> QVector<int> {
        int depth = 0;
        for (int i = position + 1; i < text.size(); ++i) {
            const QChar current = text.at(i);
            if (current == open) {
                ++depth;
            } else if (current == close) {
                if (depth == 0) {
                    return {position, i};
                }
                --depth;
            }
        }
        return {};
    };

    auto matchingCloseAt = [&](int position, QChar open, QChar close) -> QVector<int> {
        int depth = 0;
        for (int i = position - 1; i >= 0; --i) {
            const QChar current = text.at(i);
            if (current == close) {
                ++depth;
            } else if (current == open) {
                if (depth == 0) {
                    return {i, position};
                }
                --depth;
            }
        }
        return {};
    };

    auto matchingAt = [&](int position) -> QVector<int> {
        if (position < 0 || position >= text.size()) {
            return {};
        }

        const QChar current = text.at(position);
        if (isQuoteDelimiter(current)) {
            return matchingQuoteAt(position);
        }

        const QChar close = closingDelimiterFor(current);
        if (!close.isNull()) {
            return matchingOpenAt(position, current, close);
        }

        const QChar open = openingDelimiterFor(current);
        if (!open.isNull()) {
            return matchingCloseAt(position, open, current);
        }

        return {};
    };

    const int cursorPosition = textCursor().position();
    QVector<int> positions = matchingAt(cursorPosition - 1);
    if (!positions.isEmpty()) {
        return positions;
    }
    return matchingAt(cursorPosition);
}

void EditorSurface::paintIndentationGuides(QPainter *painter)
{
    if (!showIndentationGuides) {
        return;
    }

    painter->save();
    painter->setPen(QColor(86, 96, 108, 150));
    const int spaceWidth = qMax(1, fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    const int rightEdge = viewport()->rect().right() - 8;

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= viewport()->rect().bottom()) {
        if (block.isVisible() && bottom >= viewport()->rect().top()) {
            const int guideCount = indentationGuideCountForLineForTest(block.text());
            for (int level = 1; level <= guideCount; ++level) {
                const int x = rightEdge - (level * 4 * spaceWidth);
                painter->drawLine(x, top + 2, x, bottom - 2);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
    painter->restore();
}

bool EditorSurface::selectFindMatch(int index)
{
    if (index < 0 || index >= activeFindMatches.size()) {
        return false;
    }

    activeFindIndex = index;
    const EditorFindMatch match = activeFindMatches.at(index);
    QTextCursor cursor(document());
    cursor.setPosition(match.start);
    cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
    ensureCursorVisible();
    updateEditorExtraSelections();
    return true;
}

void EditorSurface::updateLineNumberAreaWidth(int)
{
    setViewportMargins(0, 0, lineNumberAreaWidth(), 0);
}

void EditorSurface::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(blockCount());
    }
}

void EditorSurface::contextMenuEvent(QContextMenuEvent *event)
{
    std::unique_ptr<QMenu> menu(createEditorContextMenu(this));
    menu->exec(event->globalPos());
}

void EditorSurface::mousePressEvent(QMouseEvent *event)
{
    if (completionPopup->isVisible()) {
        completionPopup->hide();
    }

    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        inAltColumnDrag = true;
        altColumnDragAnchor = cursorForPosition(event->pos());
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier) {
        const QTextCursor cursor = cursorForPosition(event->pos());
        emit definitionRequested(cursor.blockNumber(), cursor.position() - cursor.block().position());
        event->accept();
        return;
    }

    QPlainTextEdit::mousePressEvent(event);
}

void EditorSurface::mouseMoveEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseMoveEvent(event);
    pendingHoverViewportPosition = event->pos();
    hoverRequestTimer.start();
}

void EditorSurface::mouseReleaseEvent(QMouseEvent *event)
{
    if (inAltColumnDrag) {
        const QTextCursor releaseCursor = cursorForPosition(event->pos());
        generateColumnSelectionBetween(altColumnDragAnchor, releaseCursor);
        inAltColumnDrag = false;
        event->accept();
        return;
    }

    QPlainTextEdit::mouseReleaseEvent(event);
}

void EditorSurface::keyPressEvent(QKeyEvent *event)
{
    if (completionPopup->isVisible()) {
        if (event->key() == Qt::Key_Escape) {
            completionPopup->hide();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab) {
            insertSelectedCompletion();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up) {
            const int direction = event->key() == Qt::Key_Down ? 1 : -1;
            const int nextRow = qBound(0, completionPopup->currentRow() + direction, completionPopup->count() - 1);
            completionPopup->setCurrentRow(nextRow);
            event->accept();
            return;
        }
    }

    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::ControlModifier) {
        requestCompletionAtPrimaryCursor();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape && !secondaryCursors.isEmpty()) {
        collapseToSinglePrimaryCursor();
        event->accept();
        return;
    }

    const bool isReturn = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    const bool plainReturn = event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::KeypadModifier;
    if (secondaryCursors.isEmpty()) {
        if (!isReturn || !plainReturn) {
            const bool shouldRequestCompletion = isPlainPrintableKey(event);
            QPlainTextEdit::keyPressEvent(event);
            if (shouldRequestCompletion) {
                scheduleCompletionRequest();
            }
            return;
        }

        QTextCursor cursor = textCursor();
        cursor.insertText(QLatin1Char('\n') + returnIndentForCursor(cursor));
        setTextCursor(cursor);
        event->accept();
        return;
    }

    const Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool plainKey = modifiers == Qt::NoModifier || modifiers == Qt::KeypadModifier;
    const bool printableModifiers = plainKey
        || modifiers == Qt::ShiftModifier
        || modifiers == (Qt::ShiftModifier | Qt::KeypadModifier);
    const bool isBackspace = event->key() == Qt::Key_Backspace && plainKey;
    const bool isDelete = event->key() == Qt::Key_Delete && plainKey;
    const bool isTabIndent = event->key() == Qt::Key_Tab && plainKey;
    const bool isTabDedent = event->key() == Qt::Key_Backtab
        || (event->key() == Qt::Key_Tab && modifiers == Qt::ShiftModifier);
    const bool isArrow = plainKey
        && (event->key() == Qt::Key_Left
            || event->key() == Qt::Key_Right
            || event->key() == Qt::Key_Up
            || event->key() == Qt::Key_Down);
    const bool isPrintableInsert = printableModifiers
        && !event->text().isEmpty()
        && event->text().at(0).category() != QChar::Other_Control
        && !isReturn;
    if (isTabIndent || isTabDedent) {
        QVector<int> blockNumbers;
        blockNumbers.reserve(totalCursorCount());
        blockNumbers.append(textCursor().blockNumber());
        for (const QTextCursor &cursor : secondaryCursors) {
            blockNumbers.append(cursor.blockNumber());
        }
        std::sort(blockNumbers.begin(), blockNumbers.end(), std::greater<int>());
        blockNumbers.erase(std::unique(blockNumbers.begin(), blockNumbers.end()), blockNumbers.end());

        QTextCursor editBlockCursor = textCursor();
        editBlockCursor.beginEditBlock();
        for (const int blockNumber : blockNumbers) {
            if (isTabIndent) {
                indentBlockByOneLevel(document(), blockNumber);
            } else {
                dedentBlockByOneLevel(document(), blockNumber);
            }
        }
        editBlockCursor.endEditBlock();

        viewport()->update();
        event->accept();
        return;
    }

    if (!isPrintableInsert && !isBackspace && !isDelete && !isArrow && !(isReturn && plainReturn)) {
        // Slice-9 limitation: complex editor commands still apply only to the primary cursor.
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    struct CursorEditItem
    {
        QTextCursor cursor;
        int secondaryIndex = -1;
        bool primary = false;
    };

    QVector<CursorEditItem> items;
    items.reserve(totalCursorCount());
    items.push_back({textCursor(), -1, true});
    for (int i = 0; i < secondaryCursors.size(); ++i) {
        items.push_back({secondaryCursors.at(i), i, false});
    }
    std::sort(items.begin(), items.end(), [](const CursorEditItem &left, const CursorEditItem &right) {
        return left.cursor.position() > right.cursor.position();
    });

    QVector<QTextCursor> updatedSecondaries = secondaryCursors;
    QTextCursor updatedPrimary = textCursor();
    QTextCursor editBlockCursor = textCursor();
    editBlockCursor.beginEditBlock();
    for (CursorEditItem &item : items) {
        QTextCursor cursor = item.cursor;
        if (isPrintableInsert) {
            cursor.insertText(event->text());
        } else if (isBackspace) {
            if (cursor.hasSelection()) {
                cursor.removeSelectedText();
            } else {
                cursor.deletePreviousChar();
            }
        } else if (isDelete) {
            if (cursor.hasSelection()) {
                cursor.removeSelectedText();
            } else {
                cursor.deleteChar();
            }
        } else if (isReturn && plainReturn) {
            cursor.insertText(QLatin1Char('\n') + returnIndentForCursor(cursor));
        } else if (isArrow) {
            QTextCursor::MoveOperation operation = QTextCursor::NoMove;
            if (event->key() == Qt::Key_Left) {
                operation = QTextCursor::Left;
            } else if (event->key() == Qt::Key_Right) {
                operation = QTextCursor::Right;
            } else if (event->key() == Qt::Key_Up) {
                operation = QTextCursor::Up;
            } else if (event->key() == Qt::Key_Down) {
                operation = QTextCursor::Down;
            }
            cursor.clearSelection();
            cursor.movePosition(operation);
        }

        if (item.primary) {
            updatedPrimary = cursor;
        } else if (item.secondaryIndex >= 0 && item.secondaryIndex < updatedSecondaries.size()) {
            updatedSecondaries[item.secondaryIndex] = cursor;
        }
    }
    editBlockCursor.endEditBlock();

    secondaryCursors = updatedSecondaries;
    setTextCursor(updatedPrimary);
    viewport()->update();
    event->accept();
}

void EditorSurface::requestCompletionAtPrimaryCursor()
{
    const QTextCursor cursor = textCursor();
    emit completionRequested(cursor.blockNumber(), cursor.position() - cursor.block().position());
}

void EditorSurface::scheduleCompletionRequest()
{
    completionRequestTimer.start();
}

void EditorSurface::requestHoverAtViewportPosition(const QPoint &position)
{
    const QTextCursor cursor = cursorForPosition(position);
    emit hoverRequested(cursor.blockNumber(), cursor.position() - cursor.block().position(), position);
}

void EditorSurface::insertSelectedCompletion()
{
    if (!completionPopup || !completionPopup->isVisible() || !completionPopup->currentItem()) {
        return;
    }
    const QString insertText = completionPopup->currentItem()->data(Qt::UserRole).toString();
    completionPopup->hide();
    if (insertText.isEmpty()) {
        return;
    }

    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString line = block.text();
    const int column = qBound(0, cursor.position() - block.position(), line.size());
    int prefixStartColumn = column;
    while (prefixStartColumn > 0 && isIdentifierCharacter(line.at(prefixStartColumn - 1))) {
        --prefixStartColumn;
    }
    if (prefixStartColumn < column) {
        cursor.setPosition(block.position() + prefixStartColumn);
        cursor.setPosition(block.position() + column, QTextCursor::KeepAnchor);
    }
    cursor.insertText(insertText);
    setTextCursor(cursor);
}

void EditorSurface::inputMethodEvent(QInputMethodEvent *event)
{
    if (secondaryCursors.isEmpty()) {
        QPlainTextEdit::inputMethodEvent(event);
        return;
    }

    const QString committedText = event->commitString();
    if (committedText.isEmpty()) {
        event->accept();
        return;
    }

    struct CursorEditItem
    {
        QTextCursor cursor;
        int secondaryIndex = -1;
        bool primary = false;
    };

    QVector<CursorEditItem> items;
    items.reserve(totalCursorCount());
    items.push_back({textCursor(), -1, true});
    for (int i = 0; i < secondaryCursors.size(); ++i) {
        items.push_back({secondaryCursors.at(i), i, false});
    }
    std::sort(items.begin(), items.end(), [event](const CursorEditItem &left, const CursorEditItem &right) {
        return left.cursor.position() + event->replacementStart()
            > right.cursor.position() + event->replacementStart();
    });

    QVector<QTextCursor> updatedSecondaries = secondaryCursors;
    QTextCursor updatedPrimary = textCursor();
    QTextCursor editBlockCursor = textCursor();
    editBlockCursor.beginEditBlock();
    for (CursorEditItem &item : items) {
        QTextCursor cursor = item.cursor;
        if (event->replacementStart() != 0 || event->replacementLength() > 0) {
            const int replacementStart = qBound(0,
                cursor.position() + event->replacementStart(),
                document()->characterCount() - 1);
            const int replacementEnd = qBound(0,
                replacementStart + event->replacementLength(),
                document()->characterCount() - 1);
            cursor.setPosition(replacementStart);
            cursor.setPosition(replacementEnd, QTextCursor::KeepAnchor);
        }
        cursor.insertText(committedText);

        if (item.primary) {
            updatedPrimary = cursor;
        } else if (item.secondaryIndex >= 0 && item.secondaryIndex < updatedSecondaries.size()) {
            updatedSecondaries[item.secondaryIndex] = cursor;
        }
    }
    editBlockCursor.endEditBlock();

    secondaryCursors = updatedSecondaries;
    setTextCursor(updatedPrimary);
    viewport()->update();
    event->accept();
}

void EditorSurface::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    QPainter guidePainter(viewport());
    if (!secondaryCursors.isEmpty()) {
        paintSecondaryCarets(&guidePainter);
    }
    paintIndentationGuides(&guidePainter);

    if (!toPlainText().isEmpty() || emptyPlaceholderText.isEmpty()) {
        return;
    }

    QPainter painter(viewport());
    painter.setPen(QColor(145, 155, 160));
    const QRect textRect = viewport()->rect().adjusted(12, 8, -12, 0);
    const int textWidth = fontMetrics().horizontalAdvance(emptyPlaceholderText);
    const int x = qMax(textRect.left(), textRect.right() - textWidth + 1);
    const int y = textRect.top() + fontMetrics().ascent();
    painter.drawText(x, y, emptyPlaceholderText);
}

void EditorSurface::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    const int width = lineNumberAreaWidth();
    lineNumberArea->setGeometry(QRect(cr.right() - width + 1, cr.top(), width, cr.height()));
}
