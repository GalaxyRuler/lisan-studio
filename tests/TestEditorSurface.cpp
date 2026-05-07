#include <QtTest/QtTest>

#include "ApyHighlighter.h"
#include "EditorSurface.h"

class TestEditorSurface : public QObject
{
    Q_OBJECT

private slots:
    void preservesMixedArabicCodeText();
    void rejectsHiddenBidiControls();
    void highlightsArabicKeywordsStringsCommentsAndNumbers();
    void supportsCursorSelectionUndoAndDeleteInMixedText();
};

void TestEditorSurface::preservesMixedArabicCodeText()
{
    EditorSurface editor;
    const QString text = QString::fromUtf8(
        "# apython: dict=ar-v2\n"
        "عدد = 1\n"
        "path = \"C:/Users/Admin/مشروع/main.apy\"\n"
        "اذا عدد > 0:\n"
        "    اطبع(\"مرحبا hello 123\")\n");

    editor.setPlainText(text);

    QCOMPARE(editor.toPlainText(), text);
    QCOMPARE(editor.document()->defaultTextOption().textDirection(), Qt::RightToLeft);
}

void TestEditorSurface::rejectsHiddenBidiControls()
{
    EditorSurface editor;
    const QString unsafe = QString::fromUtf8("اسم = 1\u202E\n");

    const auto findings = editor.findHiddenBidiControls(unsafe);

    QCOMPARE(findings.size(), 1);
    QCOMPARE(findings.first().unicodeName, QStringLiteral("RIGHT-TO-LEFT OVERRIDE"));
}

void TestEditorSurface::highlightsArabicKeywordsStringsCommentsAndNumbers()
{
    const QString text = QString::fromUtf8("# تعليق\nاذا عدد == 12:\n    اطبع(\"مرحبا\")\n");

    const auto spans = ApyHighlighter::classifyLineForTest(text.section('\n', 1, 1));

    QVERIFY(std::any_of(spans.begin(), spans.end(), [](const HighlightSpan &span) {
        return span.kind == HighlightKind::Keyword && span.text == QString::fromUtf8("اذا");
    }));
    QVERIFY(std::any_of(spans.begin(), spans.end(), [](const HighlightSpan &span) {
        return span.kind == HighlightKind::Number && span.text == QStringLiteral("12");
    }));

    const auto commentSpans = ApyHighlighter::classifyLineForTest(text.section('\n', 0, 0));
    QCOMPARE(commentSpans.first().kind, HighlightKind::Comment);
}

void TestEditorSurface::supportsCursorSelectionUndoAndDeleteInMixedText()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("عدد = 12\nname = عدد\n"));

    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 4);
    editor.setTextCursor(cursor);

    QCOMPARE(editor.textCursor().selectedText(), QString::fromUtf8("عدد "));

    editor.insertPlainText(QString::fromUtf8("قيمة "));
    QVERIFY(editor.toPlainText().startsWith(QString::fromUtf8("قيمة = 12")));

    editor.undo();
    QVERIFY(editor.toPlainText().startsWith(QString::fromUtf8("عدد = 12")));
}

QTEST_MAIN(TestEditorSurface)
#include "TestEditorSurface.moc"
