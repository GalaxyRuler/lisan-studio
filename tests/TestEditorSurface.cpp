#include <QtTest/QtTest>

#include "ApyHighlighter.h"
#include "EditorSurface.h"

class TestEditorSurface : public QObject
{
    Q_OBJECT

private slots:
    void preservesMixedArabicCodeText();
    void saveAndReopenPreservesUtf8TortureText();
    void rejectsHiddenBidiControls();
    void detectsAllHiddenBidiControls();
    void highlightsArabicKeywordsStringsCommentsAndNumbers();
    void supportsCursorSelectionUndoAndDeleteInMixedText();
    void supportsCopyPasteUndoRedoAndDeleteAroundMixedDirectionText();
    void cursorCanVisitEveryLogicalPositionInMixedDirectionLongLine();
    void lineNumberAreaScalesAndStaysVisibleForLongFiles();
    void lineNumbersStayOnRightEdgeForArabicEditing();
};

static QString tortureText()
{
    return QString::fromUtf8(
        "# apython: dict=ar-v2\n"
        "اسم_المستخدم = \"سارة user-42\"\n"
        "path = \"C:/Users/Admin/مشروع طويل/src/main.apy\"\n"
        "english_name = اسم_المستخدم\n"
        "اذا len(path) > 10:\n"
        "    اطبع(\"مرحبا hello 123\")\n"
        "قائمة = [1, 22, 333]\n"
        "نتيجة = english_name + \" :: \" + path\n");
}

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
    QCOMPARE(editor.lineWrapMode(), QPlainTextEdit::NoWrap);
}

void TestEditorSurface::saveAndReopenPreservesUtf8TortureText()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QString text = tortureText();

    EditorSurface editor;
    editor.setPlainText(text);
    QVERIFY(editor.saveFileAs(path));
    QCOMPARE(editor.findHiddenBidiControls(editor.toPlainText()).size(), 0);

    EditorSurface reopened;
    QVERIFY(reopened.openFile(path));
    QCOMPARE(reopened.toPlainText(), text);
    QVERIFY(!reopened.isDirty());
}

void TestEditorSurface::rejectsHiddenBidiControls()
{
    EditorSurface editor;
    const QString unsafe = QString::fromUtf8("اسم = 1\u202E\n");

    const auto findings = editor.findHiddenBidiControls(unsafe);

    QCOMPARE(findings.size(), 1);
    QCOMPARE(findings.first().unicodeName, QStringLiteral("RIGHT-TO-LEFT OVERRIDE"));
}

void TestEditorSurface::detectsAllHiddenBidiControls()
{
    EditorSurface editor;
    const QString unsafe = QString::fromUtf8("\u202A\u202B\u202C\u202D\u202E\u2066\u2067\u2068\u2069");

    const auto findings = editor.findHiddenBidiControls(unsafe);

    QCOMPARE(findings.size(), 9);
    QCOMPARE(findings.first().unicodeName, QStringLiteral("LEFT-TO-RIGHT EMBEDDING"));
    QCOMPARE(findings.last().unicodeName, QStringLiteral("POP DIRECTIONAL ISOLATE"));
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

void TestEditorSurface::supportsCopyPasteUndoRedoAndDeleteAroundMixedDirectionText()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(QString::fromUtf8("اسم = \"سارة\"\npath = \"C:/Users/Admin/مشروع/main.apy\"\n"));

    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    editor.copy();

    cursor.clearSelection();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    editor.paste();
    QVERIFY(editor.toPlainText().endsWith(QString::fromUtf8("اسم = \"سارة\"")));

    editor.undo();
    QVERIFY(!editor.toPlainText().endsWith(QString::fromUtf8("اسم = \"سارة\"")));
    editor.redo();
    QVERIFY(editor.toPlainText().endsWith(QString::fromUtf8("اسم = \"سارة\"")));

    cursor = editor.textCursor();
    const int arabicStart = editor.toPlainText().indexOf(QString::fromUtf8("سارة"));
    QVERIFY(arabicStart > 0);
    cursor.setPosition(arabicStart + 1);
    editor.setTextCursor(cursor);
    QTest::keyClick(&editor, Qt::Key_Backspace);
    QVERIFY(editor.toPlainText().contains(QString::fromUtf8("ارة")));

    cursor = editor.textCursor();
    cursor.setPosition(editor.toPlainText().indexOf(QString::fromUtf8("مشروع")));
    editor.setTextCursor(cursor);
    QTest::keyClick(&editor, Qt::Key_Delete);
    QVERIFY(editor.toPlainText().contains(QString::fromUtf8("شروع/main.apy")));
}

void TestEditorSurface::cursorCanVisitEveryLogicalPositionInMixedDirectionLongLine()
{
    EditorSurface editor;
    const QString line = QString::fromUtf8("نتيجة = call_english(اسم_المستخدم, 123, \"C:/Users/Admin/مشروع/main.apy\") + \" hello مرحبا \"");
    editor.setPlainText(line);

    QTextCursor cursor = editor.textCursor();
    for (int position = 0; position <= line.size(); ++position) {
        cursor.setPosition(position);
        editor.setTextCursor(cursor);
        QCOMPARE(editor.textCursor().position(), position);
    }

    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, line.size());
    editor.setTextCursor(cursor);
    QCOMPARE(editor.textCursor().selectedText(), line);
}

void TestEditorSurface::lineNumberAreaScalesAndStaysVisibleForLongFiles()
{
    EditorSurface editor;
    QStringList lines;
    for (int i = 1; i <= 125; ++i) {
        lines << QString::fromUtf8("اطبع(\"line %1\")").arg(i);
    }

    const int initialWidth = editor.lineNumberAreaWidth();
    editor.setPlainText(lines.join('\n'));
    const int expandedWidth = editor.lineNumberAreaWidth();

    QVERIFY(expandedWidth > initialWidth);
    editor.resize(640, 480);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    auto *lineNumberArea = editor.findChild<QWidget *>(QStringLiteral("lineNumberArea"));
    QVERIFY(lineNumberArea != nullptr);
    QVERIFY(lineNumberArea->isVisible());
    QVERIFY(lineNumberArea->width() >= expandedWidth);
}

void TestEditorSurface::lineNumbersStayOnRightEdgeForArabicEditing()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اجلب\nاطبع(\"مرحبا\")\n"));
    editor.resize(640, 360);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    auto *lineNumberArea = editor.findChild<QWidget *>(QStringLiteral("lineNumberArea"));
    QVERIFY(lineNumberArea != nullptr);
    QVERIFY(lineNumberArea->geometry().right() >= editor.contentsRect().right() - 1);
    QVERIFY(editor.viewport()->geometry().right() < lineNumberArea->geometry().left());

    const QTextOption option = editor.document()->defaultTextOption();
    QCOMPARE(option.textDirection(), Qt::RightToLeft);
    QCOMPARE(option.alignment() & Qt::AlignHorizontal_Mask, Qt::AlignRight);
}

QTEST_MAIN(TestEditorSurface)
#include "TestEditorSurface.moc"
