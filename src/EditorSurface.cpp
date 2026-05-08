#include "EditorSurface.h"

#include <QPainter>
#include <QFile>
#include <QFontDatabase>
#include <QPaintEvent>
#include <QTextBlock>
#include <QTextOption>

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
    setPlaceholderText(QString::fromUtf8("اكتب كود لغة الثعبان هنا"));

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
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }

    setPlainText(QString::fromUtf8(file.readAll()));
    document()->setModified(false);
    setCurrentFilePath(path);
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
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }

    file.write(toPlainText().toUtf8());
    document()->setModified(false);
    setCurrentFilePath(path);
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

void EditorSurface::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    const int width = lineNumberAreaWidth();
    lineNumberArea->setGeometry(QRect(cr.right() - width + 1, cr.top(), width, cr.height()));
}
