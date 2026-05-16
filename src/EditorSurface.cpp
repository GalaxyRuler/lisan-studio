#include "EditorSurface.h"

#include "DocumentFileIO.h"

#include <QPainter>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPaintEvent>
#include <QTextBlock>
#include <QTextOption>

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

private:
    EditorSurface *editorSurface = nullptr;
};

EditorSurface::EditorSurface(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setLayoutDirection(Qt::RightToLeft);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setUndoRedoEnabled(true);
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

    highlighter = new ApyHighlighter(document());

    connect(document(), &QTextDocument::modificationChanged, this, &EditorSurface::dirtyStateChanged);
    connect(document(), &QTextDocument::contentsChanged, this, [this]() {
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
    setCurrentFilePath(loaded.identity.path);
    return true;
}

void EditorSurface::resetForNewFile()
{
    clear();
    document()->setModified(false);
    setCurrentFilePath(QString());
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

    if (!DocumentFileIO::saveUtf8Atomically(path, textToSave, error)) {
        return false;
    }

    document()->setModified(false);
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

void EditorSurface::keyPressEvent(QKeyEvent *event)
{
    const bool isReturn = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    const bool plainReturn = event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::KeypadModifier;
    if (!isReturn || !plainReturn) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    QTextCursor cursor = textCursor();
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

    cursor.insertText(QLatin1Char('\n') + indent);
    setTextCursor(cursor);
    event->accept();
}

void EditorSurface::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    QPainter guidePainter(viewport());
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
