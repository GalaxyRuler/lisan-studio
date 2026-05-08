#include <QtTest/QtTest>

#include "MainWindow.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void opensProjectAndFileFromPath();
};

static QString writeFile(const QDir &root, const QString &relative, const QString &text)
{
    const QFileInfo info(root.filePath(relative));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qFatal("could not write test file");
    }
    file.write(text.toUtf8());
    return info.absoluteFilePath();
}

void TestMainWindow::opensProjectAndFileFromPath()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("src/main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(root.absolutePath()));
    QCOMPARE(window.currentProjectRoot(), root.absolutePath());

    QVERIFY(window.openPath(filePath));
    QCOMPARE(QDir::toNativeSeparators(window.currentEditorPath()), QDir::toNativeSeparators(filePath));

    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QVERIFY(editor->toPlainText().contains(QString::fromUtf8("مرحبا")));
}

QTEST_MAIN(TestMainWindow)
#include "TestMainWindow.moc"
