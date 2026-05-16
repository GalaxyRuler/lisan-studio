#include "EditorSurface.h"

#include "DocumentFileIO.h"

#include <QPainter>
#include <QFontDatabase>
#include <QKeySequence>
#include <QPaintEvent>
#include <QTextBlock>
#include <QTextOption>

#include <memory>

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
    if (!DocumentFileIO::saveUtf8Atomically(path, toPlainText(), error)) {
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

void EditorSurface::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

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
