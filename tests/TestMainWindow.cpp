#include <QtTest/QtTest>

#include "MainWindow.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void opensProjectAndFileFromPath();
    void newFileClearsCurrentPathAndEditorText();
    void projectTreeShowsOnlyFileNames();
    void outputPanelIsVisibleForRunFeedback();
    void outputPlaceholderPaintsFromRight();
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

void TestMainWindow::newFileClearsCurrentPathAndEditorText()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"مرحبا\")\n"));

    MainWindow window;
    QVERIFY(window.openPath(filePath));
    QVERIFY(!window.currentEditorPath().isEmpty());

    QVERIFY(QMetaObject::invokeMethod(&window, "newFile", Qt::DirectConnection));

    QCOMPARE(window.currentEditorPath(), QString());
    auto *editor = window.findChild<EditorSurface *>(QStringLiteral("editorSurface"));
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->toPlainText(), QString());
}

void TestMainWindow::projectTreeShowsOnlyFileNames()
{
    MainWindow window;
    auto *tree = window.findChild<QTreeView *>(QStringLiteral("projectTree"));
    QVERIFY(tree != nullptr);

    QVERIFY(!tree->isColumnHidden(0));
    QVERIFY(tree->isColumnHidden(1));
    QVERIFY(tree->isColumnHidden(2));
    QVERIFY(tree->isColumnHidden(3));
}

void TestMainWindow::outputPanelIsVisibleForRunFeedback()
{
    MainWindow window;
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);
    QVERIFY(outputPanel->isVisible());
    QVERIFY(outputPanel->height() >= 140);
}

void TestMainWindow::outputPlaceholderPaintsFromRight()
{
    MainWindow window;
    window.resize(1200, 800);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *outputPanel = window.findChild<QPlainTextEdit *>(QStringLiteral("outputPanel"));
    QVERIFY(outputPanel != nullptr);

    QImage image(outputPanel->size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    outputPanel->render(&image);

    auto countVisibleTextPixels = [&image](const QRect &rect) {
        int count = 0;
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            for (int x = rect.left(); x <= rect.right(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (color.lightness() > 110 && color.alpha() > 0) {
                    ++count;
                }
            }
        }
        return count;
    };

    const QRect content = outputPanel->viewport()->geometry();
    const QRect leftBand(content.left() + 12, content.top() + 8, 260, 44);
    const QRect rightBand(content.right() - 320, content.top() + 8, 260, 44);
    const int leftPixels = countVisibleTextPixels(leftBand);
    const int rightPixels = countVisibleTextPixels(rightBand);

    QVERIFY2(rightPixels > leftPixels * 2,
        qPrintable(QStringLiteral("Arabic output placeholder should be painted near the right writing edge. left=%1 right=%2")
            .arg(leftPixels)
            .arg(rightPixels)));
}

QTEST_MAIN(TestMainWindow)
#include "TestMainWindow.moc"
