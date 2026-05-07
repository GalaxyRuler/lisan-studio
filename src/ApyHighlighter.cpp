#include "ApyHighlighter.h"

#include <QSet>
#include <QTextDocument>

namespace
{
QTextCharFormat makeFormat(const QColor &color, bool bold = false)
{
    QTextCharFormat format;
    format.setForeground(color);
    if (bold) {
        format.setFontWeight(QFont::DemiBold);
    }
    return format;
}
}

ApyHighlighter::ApyHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
      keywordFormat(makeFormat(QColor(95, 218, 198), true)),
      stringFormat(makeFormat(QColor(242, 191, 89))),
      commentFormat(makeFormat(QColor(134, 148, 144))),
      numberFormat(makeFormat(QColor(255, 178, 182))),
      hiddenBidiFormat(makeFormat(QColor(255, 180, 171), true))
{
    hiddenBidiFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    hiddenBidiFormat.setUnderlineColor(QColor(255, 180, 171));
}

bool ApyHighlighter::isArabicKeyword(const QString &word)
{
    static const QSet<QString> keywords = {
        QString::fromUtf8("اذا"),
        QString::fromUtf8("وإلا"),
        QString::fromUtf8("والا"),
        QString::fromUtf8("طالما"),
        QString::fromUtf8("لكل"),
        QString::fromUtf8("في"),
        QString::fromUtf8("دالة"),
        QString::fromUtf8("صنف"),
        QString::fromUtf8("ارجع"),
        QString::fromUtf8("استورد"),
        QString::fromUtf8("من"),
        QString::fromUtf8("كسر"),
        QString::fromUtf8("استمر"),
        QString::fromUtf8("حاول"),
        QString::fromUtf8("امسك"),
        QString::fromUtf8("اخيرا"),
        QString::fromUtf8("مع"),
        QString::fromUtf8("باسم"),
        QString::fromUtf8("صحيح"),
        QString::fromUtf8("خطأ"),
        QString::fromUtf8("لاشيء"),
        QString::fromUtf8("اطبع"),
    };

    return keywords.contains(word);
}

bool ApyHighlighter::isHiddenBidiControl(QChar ch)
{
    const ushort value = ch.unicode();
    return (value >= 0x202A && value <= 0x202E) || (value >= 0x2066 && value <= 0x2069);
}

QVector<HighlightSpan> ApyHighlighter::classifyLineForTest(const QString &line)
{
    QVector<HighlightSpan> spans;

    const int commentStart = line.indexOf('#');
    if (commentStart >= 0) {
        spans.push_back({commentStart, static_cast<int>(line.size()) - commentStart, HighlightKind::Comment, line.mid(commentStart)});
    }

    int index = 0;
    while (index < line.size()) {
        const QChar ch = line.at(index);

        if (isHiddenBidiControl(ch)) {
            spans.push_back({index, 1, HighlightKind::HiddenBidiControl, QString(ch)});
            ++index;
            continue;
        }

        if (ch == '"' || ch == '\'') {
            const QChar quote = ch;
            int end = index + 1;
            while (end < line.size()) {
                if (line.at(end) == quote && line.at(end - 1) != '\\') {
                    ++end;
                    break;
                }
                ++end;
            }
            spans.push_back({index, end - index, HighlightKind::String, line.mid(index, end - index)});
            index = end;
            continue;
        }

        if (ch.isDigit()) {
            int end = index + 1;
            while (end < line.size() && (line.at(end).isDigit() || line.at(end) == '.')) {
                ++end;
            }
            spans.push_back({index, end - index, HighlightKind::Number, line.mid(index, end - index)});
            index = end;
            continue;
        }

        if (ch.isLetter() || ch == '_') {
            int end = index + 1;
            while (end < line.size()) {
                const QChar next = line.at(end);
                if (!(next.isLetterOrNumber() || next == '_')) {
                    break;
                }
                ++end;
            }
            const QString word = line.mid(index, end - index);
            if (isArabicKeyword(word)) {
                spans.push_back({index, end - index, HighlightKind::Keyword, word});
            }
            index = end;
            continue;
        }

        ++index;
    }

    return spans;
}

void ApyHighlighter::highlightBlock(const QString &text)
{
    const auto spans = classifyLineForTest(text);
    for (const auto &span : spans) {
        switch (span.kind) {
        case HighlightKind::Keyword:
            setFormat(span.start, span.length, keywordFormat);
            break;
        case HighlightKind::String:
            setFormat(span.start, span.length, stringFormat);
            break;
        case HighlightKind::Comment:
            setFormat(span.start, span.length, commentFormat);
            break;
        case HighlightKind::Number:
            setFormat(span.start, span.length, numberFormat);
            break;
        case HighlightKind::HiddenBidiControl:
            setFormat(span.start, span.length, hiddenBidiFormat);
            break;
        case HighlightKind::Identifier:
            break;
        }
    }
}
