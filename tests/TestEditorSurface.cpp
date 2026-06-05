#include <QtTest/QtTest>

#include "ApyHighlighter.h"
#include "EditorSurface.h"

#include <QMenu>

#include <algorithm>
#include <memory>

class TestEditorSurface : public QObject
{
    Q_OBJECT

private slots:
    void preservesMixedArabicCodeText();
    void asciiOnlyCodeUsesLtrDocumentPolicy();
    void saveAndReopenPreservesUtf8TortureText();
    void saveFileRoundTripsLoadedCrlfLineEndings();
    void editorSurfaceRoundTripsCrlf();
    void editorSurfaceRoundTripsLfAndDoesNotIntroduceCarriageReturns();
    void editorSurfaceAppliesPersistedEndingToNewlyTypedLines();
    void editorSurfaceUsesDominantEndingForMixedFile();
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
    void addCursorAtPositionRespectsHardCapAndReturnsFalse();
    void addCursorAtPositionDedupesAgainstPrimaryAndSecondaries();
    void addCursorAboveAndBelowPreserveColumn();
    void collapseToSinglePrimaryCursorClearsAllSecondaries();
    void typingWithMultipleCursorsInsertsAtAllPositions();
    void multiCursorUndoRevertsAllCursorEditsInOneStep();
    void multiCursorUndoRedoStormPreservesPositionsAndDirtyState();
    void multiCursorTabIndentsEachCursorLineByOneIndent();
    void multiCursorShiftTabDedentsEachCursorLineByOneIndent();
    void multiCursorIndentIsSingleUndoStep();
    void arabicImeCompositionProducesIdenticalCommittedTextAtEveryCursor();
    void imeCompositionWithSecondaryCursorsPreservesSingleUndoStep();
    void completionRequestSignalReportsCursorPosition();
    void completionPopupDisplaysItemsAndAcceptsSelection();
    void hoverTooltipSurfaceStoresMarkdown();
    void semanticTokensLayerOnTopOfApyHighlighter();
    void lineNumberMarginClickTogglesBreakpoints();
    void ctrlClickRequestsDefinitionAtIdentifier();
    void selectAllFindMatchesAsCursorsConvertsFindHighlights();
    void altColumnDragGeneratesOneCursorPerLineInRectangle();
    void altColumnDragWithZeroWidthColumnsGeneratesZeroWidthCursors();
    void altColumnDragRespectsLineLengthClamping();
    void altColumnDragRespectsHardCursorCap();
    void altLeftMousePressThenReleaseInvokesColumnGeneration();
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

static QByteArray readFileBytes(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("could not read test file");
    }
    return file.readAll();
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

void TestEditorSurface::asciiOnlyCodeUsesLtrDocumentPolicy()
{
    EditorSurface editor;
    const QString text = QStringLiteral(
        "name = 1\n"
        "print(name)\n");

    editor.setPlainText(text);

    const QTextOption option = editor.document()->defaultTextOption();
    QCOMPARE(option.textDirection(), Qt::LeftToRight);
    QCOMPARE(option.alignment(), Qt::AlignLeft);
    QCOMPARE(editor.layoutDirection(), Qt::RightToLeft);
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

void TestEditorSurface::saveFileRoundTripsLoadedCrlfLineEndings()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QByteArray original = QString::fromUtf8(
        "عدد = 1\r\n"
        "    اطبع(عدد)\r\n").toUtf8();

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();

    EditorSurface editor;
    QVERIFY(editor.openFile(path));
    QVERIFY(editor.saveFile());

    QCOMPARE(readFileBytes(path), original);
}

void TestEditorSurface::editorSurfaceRoundTripsCrlf()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QByteArray original = QString::fromUtf8(
        "اسم = \"سارة\"\r\n"
        "اطبع(اسم)\r\n").toUtf8();

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();

    EditorSurface editor;
    QVERIFY(editor.openFile(path));
    QVERIFY(editor.saveFile());

    QCOMPARE(readFileBytes(path), original);
}

void TestEditorSurface::editorSurfaceRoundTripsLfAndDoesNotIntroduceCarriageReturns()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QByteArray original = QString::fromUtf8(
        "اسم = \"سارة\"\n"
        "اطبع(اسم)\n").toUtf8();

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();

    EditorSurface editor;
    QVERIFY(editor.openFile(path));
    QVERIFY(editor.saveFile());

    const QByteArray saved = readFileBytes(path);
    QCOMPARE(saved, original);
    QVERIFY(!saved.contains('\r'));
}

void TestEditorSurface::editorSurfaceAppliesPersistedEndingToNewlyTypedLines()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QByteArray original = QString::fromUtf8(
        "اسم = \"سارة\"\r\n"
        "اطبع(اسم)\r\n").toUtf8();

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();

    EditorSurface editor;
    QVERIFY(editor.openFile(path));
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    editor.insertPlainText(QString::fromUtf8("اطبع(اسم2)\n"));

    QVERIFY(editor.saveFile());

    const QByteArray expected = original + QString::fromUtf8("اطبع(اسم2)\r\n").toUtf8();
    QCOMPARE(readFileBytes(path), expected);
}

void TestEditorSurface::editorSurfaceUsesDominantEndingForMixedFile()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("main.apy"));
    const QByteArray original = QString::fromUtf8(
        "اسم = \"سارة\"\r\n"
        "عدد = 1\r\n"
        "اطبع(اسم)\n").toUtf8();

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(original), original.size());
    file.close();

    EditorSurface editor;
    QVERIFY(editor.openFile(path));
    QVERIFY(editor.saveFile());

    const QByteArray expected = QString::fromUtf8(
        "اسم = \"سارة\"\r\n"
        "عدد = 1\r\n"
        "اطبع(اسم)\r\n").toUtf8();
    QCOMPARE(readFileBytes(path), expected);
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
    QCOMPARE(saved.readAll(), QString::fromUtf8("اطبع(\"مرحبا\")\r\n").toUtf8());

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
    QCOMPARE(saved.readAll(), QString::fromUtf8("عدد = 1   \r\n    اطبع(عدد)\t \r\n").toUtf8());
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
    QCOMPARE(saved.readAll(), QString::fromUtf8("عدد = 1\r\n    اطبع(عدد)\r\n").toUtf8());
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

static QTextCursor cursorAtLineColumn(EditorSurface &editor, int line, int column)
{
    QTextBlock block = editor.document()->findBlockByNumber(line);
    Q_ASSERT(block.isValid());
    QTextCursor cursor(block);
    cursor.setPosition(block.position() + qMin(column, block.length() - 1));
    return cursor;
}

static QVector<QTextCursor> allTestCursors(const EditorSurface &editor)
{
    QVector<QTextCursor> cursors;
    cursors.append(editor.textCursor());
    cursors += editor.secondaryCursorsForTest();
    std::sort(cursors.begin(), cursors.end(), [](const QTextCursor &left, const QTextCursor &right) {
        return left.blockNumber() < right.blockNumber();
    });
    return cursors;
}

void TestEditorSurface::addCursorAtPositionRespectsHardCapAndReturnsFalse()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("x").repeated(2000));

    for (int position = 1; position < EditorSurface::kHardCursorCap; ++position) {
        QVERIFY(editor.addCursorAtPosition(position));
    }

    QCOMPARE(editor.totalCursorCount(), EditorSurface::kHardCursorCap);
    QVERIFY(!editor.addCursorAtPosition(EditorSurface::kHardCursorCap));
}

void TestEditorSurface::addCursorAtPositionDedupesAgainstPrimaryAndSecondaries()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("0123456789012345678901234567890123456789"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(10);
    editor.setTextCursor(cursor);

    QVERIFY(editor.addCursorAtPosition(20));
    QVERIFY(!editor.addCursorAtPosition(10));
    QVERIFY(!editor.addCursorAtPosition(20));
    QVERIFY(editor.addCursorAtPosition(30));
    QCOMPARE(editor.totalCursorCount(), 3);
}

void TestEditorSurface::addCursorAboveAndBelowPreserveColumn()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("zero\none1\ntwo2\nthree\nfour\n"));

    QTextCursor cursor(editor.document()->findBlockByNumber(2));
    cursor.setPosition(cursor.block().position() + 3);
    editor.setTextCursor(cursor);

    QVERIFY(editor.addCursorAbovePrimary());
    QVERIFY(editor.addCursorBelowPrimary());

    const QVector<QTextCursor> secondaries = editor.secondaryCursorsForTest();
    QCOMPARE(secondaries.size(), 2);
    QCOMPARE(secondaries.at(0).blockNumber(), 1);
    QCOMPARE(secondaries.at(0).position() - secondaries.at(0).block().position(), 3);
    QVERIFY(!secondaries.at(0).hasSelection());
    QCOMPARE(secondaries.at(1).blockNumber(), 3);
    QCOMPARE(secondaries.at(1).position() - secondaries.at(1).block().position(), 3);
    QVERIFY(!secondaries.at(1).hasSelection());
}

void TestEditorSurface::collapseToSinglePrimaryCursorClearsAllSecondaries()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("abcdef"));
    QVERIFY(editor.addCursorAtPosition(1));
    QVERIFY(editor.addCursorAtPosition(2));
    QVERIFY(editor.addCursorAtPosition(3));

    editor.collapseToSinglePrimaryCursor();

    QCOMPARE(editor.totalCursorCount(), 1);
    QVERIFY(editor.secondaryCursorsForTest().isEmpty());
}

void TestEditorSurface::typingWithMultipleCursorsInsertsAtAllPositions()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    const QString original = QStringLiteral("line one\nline two\nline three\n");
    editor.setPlainText(original);

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock lineTwo = editor.document()->findBlockByNumber(1);
    const QTextBlock lineThree = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(lineTwo.position() + lineTwo.length() - 1));
    QVERIFY(editor.addCursorAtPosition(lineThree.position() + lineThree.length() - 1));

    QTest::keyClicks(&editor, QStringLiteral("X"));

    QCOMPARE(editor.toPlainText(), QStringLiteral("line oneX\nline twoX\nline threeX\n"));
    editor.undo();
    QCOMPARE(editor.toPlainText(), original);
}

void TestEditorSurface::multiCursorUndoRevertsAllCursorEditsInOneStep()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    const QString original = QStringLiteral("first\nsecond\nthird\n");
    editor.setPlainText(original);
    editor.document()->setModified(false);

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    QTest::keyClicks(&editor, QStringLiteral("X"));
    QCOMPARE(editor.toPlainText(), QStringLiteral("firstX\nsecondX\nthirdX\n"));
    QVERIFY(editor.document()->isModified());

    QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
    QCOMPARE(editor.toPlainText(), original);
    QVERIFY(!editor.document()->isModified());

    QTest::keyClick(&editor, Qt::Key_Y, Qt::ControlModifier);
    QCOMPARE(editor.toPlainText(), QStringLiteral("firstX\nsecondX\nthirdX\n"));
    QVERIFY(editor.document()->isModified());
}

void TestEditorSurface::multiCursorUndoRedoStormPreservesPositionsAndDirtyState()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    const QString original = QStringLiteral("alpha\nbeta\ngamma\n");
    editor.setPlainText(original);
    editor.document()->setModified(false);

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    auto cursorPositions = [&editor]() {
        QVector<int> positions;
        positions.append(editor.textCursor().position());
        const QVector<QTextCursor> secondaries = editor.secondaryCursorsForTest();
        for (const QTextCursor &secondary : secondaries) {
            positions.append(secondary.position());
        }
        std::sort(positions.begin(), positions.end());
        return positions;
    };

    const QVector<int> cleanPositions = cursorPositions();
    for (int i = 0; i < 50; ++i) {
        QTest::keyClicks(&editor, QStringLiteral("X"));
        const QVector<int> editedPositions = cursorPositions();
        QCOMPARE(editor.toPlainText(), QStringLiteral("alphaX\nbetaX\ngammaX\n"));
        QVERIFY(editor.document()->isModified());

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText(), original);
        QCOMPARE(cursorPositions(), cleanPositions);
        QVERIFY(!editor.document()->isModified());

        QTest::keyClick(&editor, Qt::Key_Y, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText(), QStringLiteral("alphaX\nbetaX\ngammaX\n"));
        QCOMPARE(cursorPositions(), editedPositions);
        QVERIFY(editor.document()->isModified());

        QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(editor.toPlainText(), original);
        QCOMPARE(cursorPositions(), cleanPositions);
        QVERIFY(!editor.document()->isModified());
    }
}

void TestEditorSurface::multiCursorTabIndentsEachCursorLineByOneIndent()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(QStringLiteral("alpha\nbeta\ngamma\n"));

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    QTest::keyClick(&editor, Qt::Key_Tab);

    QCOMPARE(editor.toPlainText(), QStringLiteral("    alpha\n    beta\n    gamma\n"));
}

void TestEditorSurface::multiCursorShiftTabDedentsEachCursorLineByOneIndent()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(QStringLiteral("    alpha\n    beta\n    gamma\n"));

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    QTest::keyClick(&editor, Qt::Key_Backtab, Qt::ShiftModifier);

    QCOMPARE(editor.toPlainText(), QStringLiteral("alpha\nbeta\ngamma\n"));
}

void TestEditorSurface::multiCursorIndentIsSingleUndoStep()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    const QString original = QStringLiteral("alpha\nbeta\ngamma\n");
    editor.setPlainText(original);
    editor.document()->setModified(false);

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    QTest::keyClick(&editor, Qt::Key_Tab);
    QCOMPARE(editor.toPlainText(), QStringLiteral("    alpha\n    beta\n    gamma\n"));
    QVERIFY(editor.document()->isModified());

    QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
    QCOMPARE(editor.toPlainText(), original);
    QVERIFY(!editor.document()->isModified());
}

void TestEditorSurface::arabicImeCompositionProducesIdenticalCommittedTextAtEveryCursor()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(QString::fromUtf8("سطر1\nسطر2\nسطر3\n"));

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    QInputMethodEvent preedit(QString::fromUtf8("مرح"), {});
    QApplication::sendEvent(&editor, &preedit);

    QInputMethodEvent commit;
    commit.setCommitString(QString::fromUtf8("مرحبا"));
    QApplication::sendEvent(&editor, &commit);

    QCOMPARE(editor.toPlainText(), QString::fromUtf8("سطر1مرحبا\nسطر2مرحبا\nسطر3مرحبا\n"));
}

void TestEditorSurface::imeCompositionWithSecondaryCursorsPreservesSingleUndoStep()
{
    EditorSurface editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    const QString original = QString::fromUtf8("سطر1\nسطر2\nسطر3\n");
    editor.setPlainText(original);
    editor.document()->setModified(false);

    QTextCursor cursor(editor.document()->findBlockByNumber(0));
    cursor.movePosition(QTextCursor::EndOfBlock);
    editor.setTextCursor(cursor);
    const QTextBlock second = editor.document()->findBlockByNumber(1);
    const QTextBlock third = editor.document()->findBlockByNumber(2);
    QVERIFY(editor.addCursorAtPosition(second.position() + second.length() - 1));
    QVERIFY(editor.addCursorAtPosition(third.position() + third.length() - 1));

    QInputMethodEvent commit;
    commit.setCommitString(QString::fromUtf8("مرحبا"));
    QApplication::sendEvent(&editor, &commit);
    QCOMPARE(editor.toPlainText(), QString::fromUtf8("سطر1مرحبا\nسطر2مرحبا\nسطر3مرحبا\n"));
    QVERIFY(editor.document()->isModified());

    QTest::keyClick(&editor, Qt::Key_Z, Qt::ControlModifier);
    QCOMPARE(editor.toPlainText(), original);
    QVERIFY(!editor.document()->isModified());
}

void TestEditorSurface::completionRequestSignalReportsCursorPosition()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("س = اط\n"));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    QTextCursor cursor = cursorAtLineColumn(editor, 0, 6);
    editor.setTextCursor(cursor);

    QSignalSpy spy(&editor, &EditorSurface::completionRequested);
    QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
    QCOMPARE(spy.at(0).at(1).toInt(), 6);
}

void TestEditorSurface::completionPopupDisplaysItemsAndAcceptsSelection()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("س = اط"));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    QTextCursor cursor = cursorAtLineColumn(editor, 0, 6);
    editor.setTextCursor(cursor);

    QVector<EditorCompletionItem> items;
    items.append({QString::fromUtf8("اطبع"), QStringLiteral("print(value)"), QString::fromUtf8("اطبع")});
    items.append({QString::fromUtf8("اذا"), QStringLiteral("conditional"), QString::fromUtf8("اذا")});

    editor.showCompletionItems(items);

    QVERIFY(editor.isCompletionPopupVisibleForTest());
    QCOMPARE(editor.completionLabelsForTest(), QStringList({QString::fromUtf8("اطبع"), QString::fromUtf8("اذا")}));

    QTest::keyClick(&editor, Qt::Key_Return);

    QVERIFY(!editor.isCompletionPopupVisibleForTest());
    QCOMPARE(editor.toPlainText(), QString::fromUtf8("س = اطبع"));
}

void TestEditorSurface::hoverTooltipSurfaceStoresMarkdown()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اطبع(\"مرحبا\")\n"));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    editor.showHoverMarkdown(QStringLiteral("**اطبع** -> `print(value)`"), QPoint(8, 8));

    QCOMPARE(editor.visibleHoverTextForTest(), QStringLiteral("**اطبع** -> `print(value)`"));
}

void TestEditorSurface::semanticTokensLayerOnTopOfApyHighlighter()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("دالة اجمع(س):\n    ارجع س\n"));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    QVector<EditorSemanticToken> tokens;
    tokens.append({0, 5, 4, QStringLiteral("function")});
    tokens.append({1, 9, 1, QStringLiteral("variable")});
    editor.setSemanticTokens(tokens);

    QCOMPARE(editor.semanticTokenSelectionCountForTest(), 2);

    editor.setSemanticTokens({});
    QCOMPARE(editor.semanticTokenSelectionCountForTest(), 0);
}

void TestEditorSurface::lineNumberMarginClickTogglesBreakpoints()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("س = ١\nاطبع(س)\n"));
    editor.resize(640, 360);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    auto *lineNumbers = editor.findChild<QWidget *>(QStringLiteral("lineNumberArea"));
    QVERIFY(lineNumbers != nullptr);
    QSignalSpy spy(&editor, &EditorSurface::breakpointToggled);

    QTest::mouseClick(lineNumbers, Qt::LeftButton, Qt::NoModifier, QPoint(lineNumbers->width() / 2, editor.fontMetrics().height() / 2));

    QVERIFY(editor.hasBreakpointAtLine(1));
    QCOMPARE(editor.breakpointLinesForTest(), QVector<int>({1}));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.first().at(0).toInt(), 1);
    QVERIFY(spy.first().at(1).toBool());

    QTest::mouseClick(lineNumbers, Qt::LeftButton, Qt::NoModifier, QPoint(lineNumbers->width() / 2, editor.fontMetrics().height() / 2));

    QVERIFY(!editor.hasBreakpointAtLine(1));
    QVERIFY(editor.breakpointLinesForTest().isEmpty());
    QCOMPARE(spy.size(), 2);
    QVERIFY(!spy.at(1).at(1).toBool());
}

void TestEditorSurface::ctrlClickRequestsDefinitionAtIdentifier()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اطبع(س)\n"));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    const QTextCursor cursor = cursorAtLineColumn(editor, 0, 2);
    const QPoint clickPoint = editor.cursorRect(cursor).center();
    QSignalSpy spy(&editor, &EditorSurface::definitionRequested);

    QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::ControlModifier, clickPoint);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
    QCOMPARE(spy.at(0).at(1).toInt(), 2);
    QCOMPARE(editor.totalCursorCount(), 1);
}

void TestEditorSurface::selectAllFindMatchesAsCursorsConvertsFindHighlights()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("alpha alpha alpha\nbeta\nalpha\n"));

    QCOMPARE(editor.setFindQuery(QStringLiteral("alpha")), 4);
    QCOMPARE(editor.findMatchCount(), 4);
    QCOMPARE(editor.selectAllFindMatchesAsCursors(), 3);
    QCOMPARE(editor.totalCursorCount(), 4);
}

void TestEditorSurface::altColumnDragGeneratesOneCursorPerLineInRectangle()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("0123456789\n0123456789\n0123456789\n0123456789\n0123456789\n0123456789\n"));

    QCOMPARE(editor.generateColumnSelectionBetween(cursorAtLineColumn(editor, 1, 2), cursorAtLineColumn(editor, 4, 6)), 3);
    QCOMPARE(editor.totalCursorCount(), 4);

    const QVector<QTextCursor> cursors = allTestCursors(editor);
    QCOMPARE(cursors.size(), 4);
    for (int i = 0; i < cursors.size(); ++i) {
        QCOMPARE(cursors.at(i).blockNumber(), i + 1);
        QCOMPARE(cursors.at(i).selectionStart() - cursors.at(i).block().position(), 2);
        QCOMPARE(cursors.at(i).selectionEnd() - cursors.at(i).block().position(), 6);
        QCOMPARE(cursors.at(i).selectedText(), QStringLiteral("2345"));
    }
}

void TestEditorSurface::altColumnDragWithZeroWidthColumnsGeneratesZeroWidthCursors()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("0123456789\n0123456789\n0123456789\n0123456789\n0123456789\n0123456789\n"));

    QCOMPARE(editor.generateColumnSelectionBetween(cursorAtLineColumn(editor, 1, 3), cursorAtLineColumn(editor, 4, 3)), 3);
    QCOMPARE(editor.totalCursorCount(), 4);

    const QVector<QTextCursor> cursors = allTestCursors(editor);
    QCOMPARE(cursors.size(), 4);
    for (int i = 0; i < cursors.size(); ++i) {
        QCOMPARE(cursors.at(i).blockNumber(), i + 1);
        QCOMPARE(cursors.at(i).anchor(), cursors.at(i).position());
        QCOMPARE(cursors.at(i).position() - cursors.at(i).block().position(), 3);
    }
}

void TestEditorSurface::altColumnDragRespectsLineLengthClamping()
{
    EditorSurface editor;
    editor.setPlainText(QStringLiteral("abc\n0123456789\n"));

    QCOMPARE(editor.generateColumnSelectionBetween(cursorAtLineColumn(editor, 0, 0), cursorAtLineColumn(editor, 1, 8)), 1);
    QCOMPARE(editor.totalCursorCount(), 2);

    const QVector<QTextCursor> cursors = allTestCursors(editor);
    QCOMPARE(cursors.size(), 2);
    QCOMPARE(cursors.at(0).blockNumber(), 0);
    QCOMPARE(cursors.at(0).selectionStart() - cursors.at(0).block().position(), 0);
    QCOMPARE(cursors.at(0).selectionEnd() - cursors.at(0).block().position(), 3);
    QCOMPARE(cursors.at(0).selectedText(), QStringLiteral("abc"));
    QCOMPARE(cursors.at(1).blockNumber(), 1);
    QCOMPARE(cursors.at(1).selectionStart() - cursors.at(1).block().position(), 0);
    QCOMPARE(cursors.at(1).selectionEnd() - cursors.at(1).block().position(), 8);
    QCOMPARE(cursors.at(1).selectedText(), QStringLiteral("01234567"));
}

void TestEditorSurface::altColumnDragRespectsHardCursorCap()
{
    QStringList lines;
    for (int i = 0; i < 1500; ++i) {
        lines.append(QStringLiteral("0123456789"));
    }

    EditorSurface editor;
    editor.setPlainText(lines.join(QLatin1Char('\n')) + QLatin1Char('\n'));

    const int added = editor.generateColumnSelectionBetween(cursorAtLineColumn(editor, 0, 0), cursorAtLineColumn(editor, 1499, 2));
    QVERIFY(added <= EditorSurface::kHardCursorCap - 1);
    QCOMPARE(added, EditorSurface::kHardCursorCap - 1);
    QVERIFY(editor.totalCursorCount() <= EditorSurface::kHardCursorCap);
}

void TestEditorSurface::altLeftMousePressThenReleaseInvokesColumnGeneration()
{
    EditorSurface mouseEditor;
    mouseEditor.resize(640, 360);
    mouseEditor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mouseEditor));
    mouseEditor.setPlainText(QStringLiteral("0123456789\n0123456789\n0123456789\n0123456789\n"));

    QTextCursor anchor = cursorAtLineColumn(mouseEditor, 0, 2);
    QTextCursor release = cursorAtLineColumn(mouseEditor, 2, 5);
    const QPoint anchorPoint = mouseEditor.cursorRect(anchor).center();
    const QPoint releasePoint = mouseEditor.cursorRect(release).center();
    QTest::mousePress(mouseEditor.viewport(), Qt::LeftButton, Qt::AltModifier, anchorPoint);
    QTest::mouseRelease(mouseEditor.viewport(), Qt::LeftButton, Qt::AltModifier, releasePoint);

    EditorSurface directEditor;
    directEditor.setPlainText(QStringLiteral("0123456789\n0123456789\n0123456789\n0123456789\n"));
    directEditor.generateColumnSelectionBetween(cursorAtLineColumn(directEditor, 0, 2), cursorAtLineColumn(directEditor, 2, 5));

    QCOMPARE(mouseEditor.totalCursorCount(), directEditor.totalCursorCount());
    const QVector<QTextCursor> mouseCursors = allTestCursors(mouseEditor);
    const QVector<QTextCursor> directCursors = allTestCursors(directEditor);
    QCOMPARE(mouseCursors.size(), directCursors.size());
    for (int i = 0; i < mouseCursors.size(); ++i) {
        QCOMPARE(mouseCursors.at(i).blockNumber(), directCursors.at(i).blockNumber());
        QCOMPARE(mouseCursors.at(i).selectionStart() - mouseCursors.at(i).block().position(),
            directCursors.at(i).selectionStart() - directCursors.at(i).block().position());
        QCOMPARE(mouseCursors.at(i).selectionEnd() - mouseCursors.at(i).block().position(),
            directCursors.at(i).selectionEnd() - directCursors.at(i).block().position());
    }
}

QTEST_MAIN(TestEditorSurface)
#include "TestEditorSurface.moc"
