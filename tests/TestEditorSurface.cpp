#include <QtTest/QtTest>

#include "ApyHighlighter.h"
#include "EditorSurface.h"

#include <QMenu>

#include <memory>

class TestEditorSurface : public QObject
{
    Q_OBJECT

private slots:
    void preservesMixedArabicCodeText();
    void saveAndReopenPreservesUtf8TortureText();
    void openRejectsInvalidUtf8ByPolicy();
    void saveUsesDocumentIoAndRejectsInvalidTarget();
    void savePreservesTrailingWhitespaceByDefault();
    void saveTrimsTrailingWhitespaceWhenEnabled();
    void rejectsHiddenBidiControls();
    void detectsAllHiddenBidiControls();
    void highlightsArabicKeywordsStringsCommentsAndNumbers();
    void supportsCursorSelectionUndoAndDeleteInMixedText();
    void supportsCopyPasteUndoRedoAndDeleteAroundMixedDirectionText();
    void contextMenuUndoRedoActionsAreEnabledAndTriggerEditorCommands();
    void cursorCanVisitEveryLogicalPositionInMixedDirectionLongLine();
    void findNavigationSelectsArabicMatchesAndHighlightsAll();
    void replaceCurrentAndAllUseActiveFindMatches();
    void highlightsMatchingBracketsAndQuotesInMixedText();
    void bracketMatchingKeepsFindHighlightsVisible();
    void returnKeyIndentsAfterColonBlockLine();
    void returnKeyPreservesCurrentLineIndent();
    void visibleWhitespaceCanBeToggled();
    void indentationGuidesCanBeToggledAndComputed();
    void lineNumberAreaScalesAndStaysVisibleForLongFiles();
    void lineNumbersStayOnRightEdgeForArabicEditing();
    void emptyEditorPlaceholderPaintsFromRight();
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

void TestEditorSurface::openRejectsInvalidUtf8ByPolicy()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("bad.apy"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromHex("fffe4100"));
    file.close();

    EditorSurface editor;
    QString error;
    QVERIFY(!editor.openFile(path, &error));
    QVERIFY(error.contains(QString::fromUtf8("UTF-8")));
    QVERIFY(editor.toPlainText().isEmpty());
    QVERIFY(editor.currentFilePath().isEmpty());
}

void TestEditorSurface::saveUsesDocumentIoAndRejectsInvalidTarget()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));

    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اطبع(\"مرحبا\")\n"));
    QVERIFY(editor.saveFileAs(path));
    QVERIFY(!editor.isDirty());
    QVERIFY(QDir(temp.path()).entryList(QStringList(QStringLiteral("*.tmp")), QDir::Files).isEmpty());

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(saved.readAll(), QString::fromUtf8("اطبع(\"مرحبا\")\n").toUtf8());

    QString error;
    QVERIFY(!editor.saveFileAs(QDir(temp.path()).filePath(QStringLiteral("missing-dir/main.apy")), &error));
    QVERIFY(!error.isEmpty());
}

void TestEditorSurface::savePreservesTrailingWhitespaceByDefault()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QString text = QString::fromUtf8("عدد = 1   \n    اطبع(عدد)\t \n");

    EditorSurface editor;
    QVERIFY(!editor.trimTrailingWhitespaceOnSave());
    editor.setPlainText(text);
    QVERIFY(editor.saveFileAs(path));

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(saved.readAll()), text);
    QCOMPARE(editor.toPlainText(), text);
}

void TestEditorSurface::saveTrimsTrailingWhitespaceWhenEnabled()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));

    EditorSurface editor;
    editor.setTrimTrailingWhitespaceOnSave(true);
    QVERIFY(editor.trimTrailingWhitespaceOnSave());
    editor.setPlainText(QString::fromUtf8("عدد = 1   \n    اطبع(عدد)\t \n"));

    QVERIFY(editor.saveFileAs(path));
    QVERIFY(!editor.isDirty());

    const QString expected = QString::fromUtf8("عدد = 1\n    اطبع(عدد)\n");
    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(saved.readAll()), expected);
    QCOMPARE(editor.toPlainText(), expected);
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

void TestEditorSurface::contextMenuUndoRedoActionsAreEnabledAndTriggerEditorCommands()
{
    EditorSurface editor;
    editor.resize(640, 360);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    editor.setPlainText(QString::fromUtf8("س = 1"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    editor.insertPlainText(QString::fromUtf8("\nاطبع(س)"));

    const QString editedText = editor.toPlainText();
    QVERIFY(editedText.contains(QString::fromUtf8("اطبع")));

    std::unique_ptr<QMenu> undoMenu(editor.createEditorContextMenu());
    QVERIFY(undoMenu != nullptr);
    auto *undoAction = undoMenu->findChild<QAction *>(QStringLiteral("editorContextUndoAction"));
    QVERIFY(undoAction != nullptr);
    QVERIFY(undoAction->isEnabled());
    undoAction->trigger();

    QVERIFY(editor.toPlainText().size() < editedText.size());

    std::unique_ptr<QMenu> redoMenu(editor.createEditorContextMenu());
    QVERIFY(redoMenu != nullptr);
    auto *redoAction = redoMenu->findChild<QAction *>(QStringLiteral("editorContextRedoAction"));
    QVERIFY(redoAction != nullptr);
    QVERIFY(redoAction->isEnabled());
    redoAction->trigger();

    QCOMPARE(editor.toPlainText(), editedText);
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

void TestEditorSurface::findNavigationSelectsArabicMatchesAndHighlightsAll()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QCOMPARE(editor.setFindQuery(QString::fromUtf8("عدد")), 2);
    QCOMPARE(editor.findMatchCount(), 2);
    QCOMPARE(editor.currentFindMatchIndex(), 0);
    QCOMPARE(editor.textCursor().selectedText(), QString::fromUtf8("عدد"));
    QVERIFY(editor.findHighlightSelectionCountForTest() >= 2);

    QVERIFY(editor.selectNextFindMatch());
    QCOMPARE(editor.currentFindMatchIndex(), 1);
    QCOMPARE(editor.textCursor().selectedText(), QString::fromUtf8("عدد"));
}

void TestEditorSurface::replaceCurrentAndAllUseActiveFindMatches()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("عدد = 1\nاطبع(عدد)\n"));

    QCOMPARE(editor.setFindQuery(QString::fromUtf8("عدد")), 2);
    QVERIFY(editor.replaceCurrentFindMatch(QString::fromUtf8("قيمة")));
    QCOMPARE(editor.toPlainText(), QString::fromUtf8("قيمة = 1\nاطبع(عدد)\n"));
    QCOMPARE(editor.findMatchCount(), 1);

    QCOMPARE(editor.replaceAllFindMatches(QString::fromUtf8("قيمة")), 1);
    QCOMPARE(editor.toPlainText(), QString::fromUtf8("قيمة = 1\nاطبع(قيمة)\n"));
    QCOMPARE(editor.findMatchCount(), 0);
}

void TestEditorSurface::highlightsMatchingBracketsAndQuotesInMixedText()
{
    EditorSurface editor;
    const QString text = QString::fromUtf8("اطبع(\"مرحبا\")\nقائمة = [1, 2]\nنص = «أهلا»\n");
    editor.setPlainText(text);

    QTextCursor cursor = editor.textCursor();
    const int parenPosition = text.indexOf(QLatin1Char('('));
    QVERIFY(parenPosition >= 0);
    cursor.setPosition(parenPosition + 1);
    editor.setTextCursor(cursor);
    QCOMPARE(editor.bracketMatchSelectionCountForTest(), 2);

    const int quotePosition = text.indexOf(QLatin1Char('"'));
    QVERIFY(quotePosition >= 0);
    cursor.setPosition(quotePosition + 1);
    editor.setTextCursor(cursor);
    QCOMPARE(editor.bracketMatchSelectionCountForTest(), 2);

    const int guillemetPosition = text.indexOf(QString::fromUtf8("«"));
    QVERIFY(guillemetPosition >= 0);
    cursor.setPosition(guillemetPosition + 1);
    editor.setTextCursor(cursor);
    QCOMPARE(editor.bracketMatchSelectionCountForTest(), 2);
}

void TestEditorSurface::bracketMatchingKeepsFindHighlightsVisible()
{
    EditorSurface editor;
    const QString text = QString::fromUtf8("عدد = 1\nاطبع(عدد)\n");
    editor.setPlainText(text);
    QCOMPARE(editor.setFindQuery(QString::fromUtf8("عدد")), 2);

    QTextCursor cursor = editor.textCursor();
    const int parenPosition = text.indexOf(QLatin1Char('('));
    QVERIFY(parenPosition >= 0);
    cursor.setPosition(parenPosition + 1);
    editor.setTextCursor(cursor);

    QCOMPARE(editor.findHighlightSelectionCountForTest(), 2);
    QCOMPARE(editor.bracketMatchSelectionCountForTest(), 2);
}

void TestEditorSurface::returnKeyIndentsAfterColonBlockLine()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اذا شرط:"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);

    QTest::keyClick(&editor, Qt::Key_Return);

    QCOMPARE(editor.toPlainText(), QString::fromUtf8("اذا شرط:\n    "));
    QCOMPARE(editor.textCursor().position(), editor.toPlainText().size());
}

void TestEditorSurface::returnKeyPreservesCurrentLineIndent()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("    اطبع(\"مرحبا\")"));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);

    QTest::keyClick(&editor, Qt::Key_Return);

    QCOMPARE(editor.toPlainText(), QString::fromUtf8("    اطبع(\"مرحبا\")\n    "));
}

void TestEditorSurface::visibleWhitespaceCanBeToggled()
{
    EditorSurface editor;

    QVERIFY(editor.isVisibleWhitespaceEnabled());
    QVERIFY(editor.document()->defaultTextOption().flags() & QTextOption::ShowTabsAndSpaces);

    editor.setVisibleWhitespaceEnabled(false);
    QVERIFY(!editor.isVisibleWhitespaceEnabled());
    QVERIFY(!(editor.document()->defaultTextOption().flags() & QTextOption::ShowTabsAndSpaces));

    editor.setVisibleWhitespaceEnabled(true);
    QVERIFY(editor.isVisibleWhitespaceEnabled());
    QVERIFY(editor.document()->defaultTextOption().flags() & QTextOption::ShowTabsAndSpaces);
}

void TestEditorSurface::indentationGuidesCanBeToggledAndComputed()
{
    EditorSurface editor;

    QVERIFY(editor.indentationGuidesEnabled());
    QCOMPARE(editor.indentationGuideCountForLineForTest(QString::fromUtf8("اطبع(\"مرحبا\")")), 0);
    QCOMPARE(editor.indentationGuideCountForLineForTest(QString::fromUtf8("    اطبع(\"مرحبا\")")), 1);
    QCOMPARE(editor.indentationGuideCountForLineForTest(QString::fromUtf8("        اطبع(\"مرحبا\")")), 2);
    QCOMPARE(editor.indentationGuideCountForLineForTest(QString::fromUtf8("\tاطبع(\"مرحبا\")")), 1);

    editor.setIndentationGuidesEnabled(false);
    QVERIFY(!editor.indentationGuidesEnabled());
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

void TestEditorSurface::emptyEditorPlaceholderPaintsFromRight()
{
    EditorSurface editor;
    editor.resize(800, 360);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    QImage image(editor.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    editor.render(&image);

    const QRect content = editor.viewport()->geometry();
    const QColor background = image.pixelColor(content.center().x(), content.bottom() - 8);
    auto differsFromBackground = [&background](const QColor &color) {
        const int delta = qAbs(color.red() - background.red())
            + qAbs(color.green() - background.green())
            + qAbs(color.blue() - background.blue());
        return color.alpha() > 0 && delta > 30;
    };

    auto countVisibleTextPixels = [&image, &differsFromBackground](const QRect &rect) {
        int count = 0;
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            for (int x = rect.left(); x <= rect.right(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (differsFromBackground(color)) {
                    ++count;
                }
            }
        }
        return count;
    };

    const QRect leftBand(content.left() + 12, content.top() + 8, 240, 40);
    const QRect rightBand(content.right() - 300, content.top() + 8, 240, 40);
    const int leftPixels = countVisibleTextPixels(leftBand);
    const int rightPixels = countVisibleTextPixels(rightBand);

    QVERIFY2(rightPixels > 20 && rightPixels > leftPixels * 2,
        qPrintable(QStringLiteral("Empty Arabic placeholder should be painted near the right writing edge. left=%1 right=%2 background=%3,%4,%5")
            .arg(leftPixels)
            .arg(rightPixels)
            .arg(background.red())
            .arg(background.green())
            .arg(background.blue())));
}

QTEST_MAIN(TestEditorSurface)
#include "TestEditorSurface.moc"
