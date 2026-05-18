#include <QtTest/QtTest>

#include "EditorSurface.h"

#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QInputMethodEvent>
#include <QScrollBar>
#include <QTemporaryDir>

class TestEditorTorture : public QObject
{
    Q_OBJECT

private slots:
    void mixedDirectionCursorSelectionRemainsLogical();
    void hiddenBidiReplacementDoesNotCreateInvisibleControls();
    void largeFileOpenUndoRedoAndFindReplaceStayWithinBudgets();
    void longArabicLineMaintainsCursorScrollAndPaintSanity();
    // V1.5: multi-cursor overlay torture is deferred until multi-cursor/column-selection scope is accepted.
    void pasteFromRtlMixedSourceDoesNotInsertHiddenBidiControls();
    void undoRedoStormOfAlternatingEditsKeepsCursorSane();
    void findReplaceStormDoesNotLeakVisibleSelections();
    void inputMethodCompositionCyclesCommitCleanText();
    void softWrapMixedDirectionLogicalLinePaintsAndKeepsCursorSane();
    void indentationGuidePaintingHandlesTenThousandLineFile();
};

static QString largeMixedDirectionText(int lineCount)
{
    QString text;
    text.reserve(lineCount * 44);
    for (int i = 0; i < lineCount; ++i) {
        const QString symbol = (i % 500 == 0) ? QStringLiteral("target") : QStringLiteral("value");
        text += QString::fromUtf8("سطر_%1 = %2 + value_%3 + \"hello مرحبا\"\n").arg(i).arg(symbol, QString::number(i));
    }
    return text;
}

static QByteArray readAllBytes(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("could not read test file");
    }
    return file.readAll();
}

static bool hasPaintedPixel(const QImage &image)
{
    for (int y = 0; y < image.height(); y += qMax(1, image.height() / 12)) {
        for (int x = 0; x < image.width(); x += qMax(1, image.width() / 12)) {
            if (image.pixelColor(x, y).alpha() > 0) {
                return true;
            }
        }
    }
    return false;
}

static QImage renderEditor(EditorSurface &editor)
{
    QImage image(editor.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    editor.render(&image);
    return image;
}

void TestEditorTorture::mixedDirectionCursorSelectionRemainsLogical()
{
    EditorSurface editor;
    const QString text = QString::fromUtf8("نتيجة = call_english(اسم, 123, \"C:/Users/Admin/مشروع/main.apy\")\n");
    editor.setPlainText(text);

    QTextCursor cursor = editor.textCursor();
    for (int position = 0; position <= text.size(); ++position) {
        cursor.setPosition(position);
        editor.setTextCursor(cursor);
        QCOMPARE(editor.textCursor().position(), position);
    }
}

void TestEditorTorture::hiddenBidiReplacementDoesNotCreateInvisibleControls()
{
    EditorSurface editor;
    editor.setPlainText(QString::fromUtf8("اسم = \"سارة\"\n"));
    QTextCursor cursor = editor.document()->find(QString::fromUtf8("سارة"));
    QVERIFY(!cursor.isNull());
    cursor.insertText(QString::fromUtf8("ليلى"));
    QCOMPARE(editor.findHiddenBidiControls(editor.toPlainText()).size(), 0);
}

void TestEditorTorture::largeFileOpenUndoRedoAndFindReplaceStayWithinBudgets()
{
    constexpr int lineCount = 100000;
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("large.apy"));
    const QString text = largeMixedDirectionText(lineCount);
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    file.write(text.toUtf8());
    file.close();

    EditorSurface editor;
    QElapsedTimer openTimer;
    openTimer.start();
    QString error;
    QVERIFY2(editor.openFile(path, &error), qPrintable(error));
    QVERIFY2(openTimer.elapsed() <= 2000,
        qPrintable(QStringLiteral("large file open exceeded 2000 ms budget: %1 ms").arg(openTimer.elapsed())));
    QVERIFY(editor.blockCount() >= lineCount);
    const int initialDocumentCharacters = editor.document()->characterCount();

    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    for (int i = 0; i < 200; ++i) {
        editor.insertPlainText(QStringLiteral(" // %1").arg(i));
    }
    QVERIFY(editor.toPlainText().endsWith(QStringLiteral(" // 199")));

    QElapsedTimer undoRedoTimer;
    undoRedoTimer.start();
    for (int i = 0; i < 200; ++i) {
        editor.undo();
    }
    const int afterUndoDocumentCharacters = editor.document()->characterCount();
    for (int i = 0; i < 200; ++i) {
        editor.redo();
    }
    const qint64 undoRedoElapsed = undoRedoTimer.elapsed();
    QCOMPARE(afterUndoDocumentCharacters, initialDocumentCharacters);
    QVERIFY(editor.toPlainText().endsWith(QStringLiteral(" // 199")));
    QVERIFY2(undoRedoElapsed <= 500,
        qPrintable(QStringLiteral("large file undo/redo storm exceeded 500 ms budget: %1 ms").arg(undoRedoElapsed)));

    QElapsedTimer findReplaceTimer;
    findReplaceTimer.start();
    constexpr int expectedLargeFileReplacements = lineCount / 500;
    QCOMPARE(editor.setFindQuery(QStringLiteral("target")), expectedLargeFileReplacements);
    QVERIFY(editor.findHighlightSelectionCountForTest() >= 100);
    QCOMPARE(editor.replaceAllFindMatches(QString::fromUtf8("بديل12")), expectedLargeFileReplacements);
    QCOMPARE(editor.findMatchCount(), 0);
    QCOMPARE(editor.findHighlightSelectionCountForTest(), 0);
    QVERIFY2(findReplaceTimer.elapsed() <= 1000,
        qPrintable(QStringLiteral("large file find/replace storm exceeded 1000 ms budget: %1 ms").arg(findReplaceTimer.elapsed())));
}

void TestEditorTorture::longArabicLineMaintainsCursorScrollAndPaintSanity()
{
    const QString longLine = QString::fromUtf8("مرحبا").repeated(10000);
    QCOMPARE(longLine.size(), 50000);

    QElapsedTimer timer;
    timer.start();

    EditorSurface editor;
    editor.resize(960, 320);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(longLine);
    QCOMPARE(editor.blockCount(), 1);

    QTextCursor cursor = editor.textCursor();
    const int longLineSize = static_cast<int>(longLine.size());
    const QVector<int> positions = {0, longLineSize / 2, longLineSize};
    for (const int position : positions) {
        cursor.setPosition(position);
        editor.setTextCursor(cursor);
        QCOMPARE(editor.textCursor().position(), position);
        editor.ensureCursorVisible();
    }

    if (editor.horizontalScrollBar()) {
        editor.horizontalScrollBar()->setValue(editor.horizontalScrollBar()->maximum());
        QVERIFY(editor.horizontalScrollBar()->value() >= 0);
    }

    cursor.setPosition(0);
    cursor.setPosition(longLineSize, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    QCOMPARE(editor.textCursor().selectedText(), longLine);

    const QImage image = renderEditor(editor);
    QVERIFY(hasPaintedPixel(image));
    QVERIFY2(timer.elapsed() <= 5000,
        qPrintable(QStringLiteral("50k Arabic line torture exceeded 5000 ms budget: %1 ms").arg(timer.elapsed())));
}

void TestEditorTorture::pasteFromRtlMixedSourceDoesNotInsertHiddenBidiControls()
{
    QVERIFY(QApplication::clipboard() != nullptr);
    const QString source = QString::fromUtf8("عنوان عربي من مقال ويكيبيديا: Python 3.12 ومسار C:/work/مشروع/main.apy");
    QCOMPARE(EditorSurface().findHiddenBidiControls(source).size(), 0);

    EditorSurface editor;
    QApplication::clipboard()->setText(source);
    editor.paste();

    QCOMPARE(editor.toPlainText(), source);
    QCOMPARE(editor.findHiddenBidiControls(editor.toPlainText()).size(), 0);

    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("paste.apy"));
    QString error;
    QVERIFY2(editor.saveFileAs(path, &error), qPrintable(error));
    const QByteArray savedBytes = readAllBytes(path);
    QCOMPARE(savedBytes, source.toUtf8());
}

void TestEditorTorture::undoRedoStormOfAlternatingEditsKeepsCursorSane()
{
    EditorSurface editor;
    const QString original = QString::fromUtf8("بداية\n");
    editor.setPlainText(original);
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);

    QString expected = original;
    for (int i = 0; i < 220; ++i) {
        const QString token = (i % 2 == 0) ? QString::fromUtf8("س") : QStringLiteral("x");
        editor.insertPlainText(token);
        expected += token;
        if (i % 3 == 0) {
            cursor = editor.textCursor();
            cursor.deletePreviousChar();
            editor.setTextCursor(cursor);
            expected.chop(1);
        }
    }
    QCOMPARE(editor.toPlainText(), expected);

    QElapsedTimer timer;
    timer.start();
    while (editor.document()->isUndoAvailable()) {
        editor.undo();
        QVERIFY(editor.textCursor().position() >= 0);
        QVERIFY(editor.textCursor().position() <= editor.toPlainText().size());
    }
    QCOMPARE(editor.toPlainText(), original);
    while (editor.document()->isRedoAvailable()) {
        editor.redo();
        QVERIFY(editor.textCursor().position() >= 0);
        QVERIFY(editor.textCursor().position() <= editor.toPlainText().size());
    }
    QCOMPARE(editor.toPlainText(), expected);
    QVERIFY2(timer.elapsed() <= 500,
        qPrintable(QStringLiteral("alternating undo/redo storm exceeded 500 ms budget: %1 ms").arg(timer.elapsed())));
}

void TestEditorTorture::findReplaceStormDoesNotLeakVisibleSelections()
{
    QString text;
    for (int i = 0; i < 150; ++i) {
        text += QString::fromUtf8("قيمة_%1 = needle + \"مرحبا needle\"\n").arg(i);
    }

    EditorSurface editor;
    editor.setPlainText(text);

    QElapsedTimer timer;
    timer.start();
    QCOMPARE(editor.setFindQuery(QStringLiteral("needle")), 300);
    QCOMPARE(editor.findHighlightSelectionCountForTest(), 300);
    QCOMPARE(editor.replaceAllFindMatches(QString::fromUtf8("بديل12")), 300);
    QCOMPARE(editor.findMatchCount(), 0);
    QCOMPARE(editor.findHighlightSelectionCountForTest(), 0);
    QVERIFY(!editor.toPlainText().contains(QStringLiteral("needle")));
    QVERIFY2(timer.elapsed() <= 1000,
        qPrintable(QStringLiteral("find/replace storm exceeded 1000 ms budget: %1 ms").arg(timer.elapsed())));
}

void TestEditorTorture::inputMethodCompositionCyclesCommitCleanText()
{
    EditorSurface editor;
    QString expected;
    for (int i = 0; i < 25; ++i) {
        QInputMethodEvent preedit(QString::fromUtf8("مرح"), {});
        QApplication::sendEvent(&editor, &preedit);

        QInputMethodEvent commit;
        const QString finalized = QString::fromUtf8("مرحبا%1 ").arg(i);
        commit.setCommitString(finalized);
        QApplication::sendEvent(&editor, &commit);
        expected += finalized;
    }

    QCOMPARE(editor.toPlainText(), expected);
    QCOMPARE(editor.findHiddenBidiControls(editor.toPlainText()).size(), 0);
    QVERIFY(editor.textCursor().position() >= 0);
    QVERIFY(editor.textCursor().position() <= editor.toPlainText().size());
}

void TestEditorTorture::softWrapMixedDirectionLogicalLinePaintsAndKeepsCursorSane()
{
    const QString segment = QString::fromUtf8("مرحبا English123 مسار/C:/work/مشروع ");
    QString line;
    while (line.size() < 10000) {
        line += segment;
    }
    line.truncate(10000);

    EditorSurface editor;
    editor.resize(720, 360);
    editor.setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(line);
    QCOMPARE(editor.blockCount(), 1);
    QVERIFY(editor.lineWrapMode() == QPlainTextEdit::WidgetWidth);

    QTextCursor cursor = editor.textCursor();
    for (const int position : {0, 4096, 9999, 10000}) {
        cursor.setPosition(position);
        editor.setTextCursor(cursor);
        editor.ensureCursorVisible();
        QCOMPARE(editor.textCursor().position(), position);
    }

    const QImage image = renderEditor(editor);
    QVERIFY(hasPaintedPixel(image));
}

void TestEditorTorture::indentationGuidePaintingHandlesTenThousandLineFile()
{
    constexpr int lineCount = 10000;
    QString text;
    text.reserve(lineCount * 32);
    for (int i = 0; i < lineCount; ++i) {
        const int depth = i % 5;
        text += QString(QStringLiteral(" ")).repeated(depth * 4)
            + QString::fromUtf8("اطبع(\"%1\")\n").arg(i);
    }

    QElapsedTimer timer;
    timer.start();
    EditorSurface editor;
    editor.resize(800, 420);
    editor.setIndentationGuidesEnabled(true);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setPlainText(text);
    QCOMPARE(editor.blockCount(), lineCount + 1);
    QVERIFY(editor.indentationGuidesEnabled());
    QCOMPARE(editor.indentationGuideCountForLineForTest(QStringLiteral("                value")), 4);

    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    editor.ensureCursorVisible();

    const QImage image = renderEditor(editor);
    QVERIFY(hasPaintedPixel(image));
    QVERIFY2(timer.elapsed() <= 5000,
        qPrintable(QStringLiteral("10k-line indentation-guide paint exceeded 5000 ms budget: %1 ms").arg(timer.elapsed())));
}

QTEST_MAIN(TestEditorTorture)
#include "TestEditorTorture.moc"
