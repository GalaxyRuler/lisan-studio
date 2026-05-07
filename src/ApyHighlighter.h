#pragma once

#include <QColor>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

enum class HighlightKind
{
    Keyword,
    String,
    Comment,
    Number,
    Identifier,
    HiddenBidiControl
};

struct HighlightSpan
{
    int start = 0;
    int length = 0;
    HighlightKind kind = HighlightKind::Identifier;
    QString text;
};

class ApyHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit ApyHighlighter(QTextDocument *parent);

    static QVector<HighlightSpan> classifyLineForTest(const QString &line);
    static bool isHiddenBidiControl(QChar ch);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat keywordFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat hiddenBidiFormat;

    static bool isArabicKeyword(const QString &word);
};

