#include <QtTest/QtTest>

#include "EditorSurface.h"

#include <QElapsedTimer>
#include <QImage>

class TestEditorTorture : public QObject
{
    Q_OBJECT

private slots:
    void mixedDirectionCursorSelectionRemainsLogical();
    void hiddenBidiReplacementDoesNotCreateInvisibleControls();
    void largeMixedDirectionFileHandlesEditUndoAndFindWithinBudget();
    void veryLongArabicLineMaintainsCursorSelectionAndRenderSanity();
};

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

void TestEditorTorture::largeMixedDirectionFileHandlesEditUndoAndFindWithinBudget()
{
    constexpr int lineCount = 100000;
    QString text;
    text.reserve(lineCount * 42);
    for (int i = 0; i < lineCount; ++i) {
        text += QString::fromUtf8("سطر_%1 = target_%2 + \"hello مرحبا\"\n").arg(i).arg(i);
    }

    QElapsedTimer timer;
    timer.start();

    EditorSurface editor;
    editor.setPlainText(text);
    QVERIFY(editor.blockCount() >= lineCount);

    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    editor.insertPlainText(QString::fromUtf8("اطبع(\"tail\")\n"));
    QVERIFY(editor.toPlainText().endsWith(QString::fromUtf8("اطبع(\"tail\")\n")));

    editor.undo();
    QVERIFY(!editor.toPlainText().endsWith(QString::fromUtf8("اطبع(\"tail\")\n")));
    editor.redo();
    QVERIFY(editor.toPlainText().endsWith(QString::fromUtf8("اطبع(\"tail\")\n")));

    QCOMPARE(editor.setFindQuery(QStringLiteral("target_99999")), 1);
    QVERIFY(editor.findHighlightSelectionCountForTest() >= 1);

    QVERIFY2(timer.elapsed() < 10000,
        qPrintable(QStringLiteral("large editor torture exceeded budget: %1 ms").arg(timer.elapsed())));
}

void TestEditorTorture::veryLongArabicLineMaintainsCursorSelectionAndRenderSanity()
{
    const QString longLine = QString::fromUtf8("مرحبا").repeated(10000) + QStringLiteral(" call_english(42)");

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

    cursor.setPosition(0);
    cursor.setPosition(longLineSize, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    QCOMPARE(editor.textCursor().selectedText(), longLine);

    QImage image(editor.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    editor.render(&image);
    QVERIFY(image.pixelColor(image.width() / 2, image.height() / 2).isValid());

    QVERIFY2(timer.elapsed() < 5000,
        qPrintable(QStringLiteral("long-line editor torture exceeded budget: %1 ms").arg(timer.elapsed())));
}

QTEST_MAIN(TestEditorTorture)
#include "TestEditorTorture.moc"
