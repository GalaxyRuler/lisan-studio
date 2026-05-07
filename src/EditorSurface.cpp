#include "EditorSurface.h"

#include <QFile>
#include <QFontDatabase>
#include <QTextOption>

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
    option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
    document()->setDefaultTextOption(option);

    QFont font(QStringLiteral("Cascadia Code"));
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(12);
    setFont(font);

    highlighter = new ApyHighlighter(document());

    connect(document(), &QTextDocument::modificationChanged, this, &EditorSurface::dirtyStateChanged);
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

