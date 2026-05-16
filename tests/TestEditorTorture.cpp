#include <QtTest/QtTest>

#include "EditorSurface.h"

class TestEditorTorture : public QObject
{
    Q_OBJECT

private slots:
    void mixedDirectionCursorSelectionRemainsLogical();
    void hiddenBidiReplacementDoesNotCreateInvisibleControls();
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

QTEST_MAIN(TestEditorTorture)
#include "TestEditorTorture.moc"
