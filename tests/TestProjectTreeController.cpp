#include <QtTest/QtTest>

#include "ProjectTreeController.h"

#include <QAction>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeView>

class TestProjectTreeController : public QObject
{
    Q_OBJECT

private slots:
    void projectTreeControllerEmitsOpenRequestOnDoubleClick();
    void projectTreeControllerRenameEmitsPathRenamedAndUpdatesDisk();
    void projectTreeControllerDeleteEmitsPathDeletedAndRemovesFromDisk();
    void projectTreeControllerCurrentSelectionPathReturnsAbsolutePath();
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

static QModelIndex waitForIndex(QFileSystemModel &model, const QString &path)
{
    QElapsedTimer timer;
    timer.start();
    QModelIndex index;
    while (!(index = model.index(path)).isValid() && timer.elapsed() < 5000) {
        QTest::qWait(50);
    }
    if (!index.isValid()) {
        qFatal("file system model index did not become valid");
    }
    return index.siblingAtColumn(0);
}

void TestProjectTreeController::projectTreeControllerEmitsOpenRequestOnDoubleClick()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("main.apy"), QString::fromUtf8("اطبع(\"من الشجرة\")\n"));

    QTreeView tree;
    QFileSystemModel model;
    ProjectTreeController controller(&tree, &model);
    controller.setProjectRoot(root.absolutePath());
    QSignalSpy openSpy(&controller, &ProjectTreeController::openPathRequested);

    const QModelIndex fileIndex = waitForIndex(model, filePath);
    tree.setCurrentIndex(fileIndex);

    QVERIFY(QMetaObject::invokeMethod(
        &tree,
        "doubleClicked",
        Qt::DirectConnection,
        Q_ARG(QModelIndex, fileIndex)));

    QCOMPARE(openSpy.count(), 1);
    QCOMPARE(QDir::toNativeSeparators(openSpy.takeFirst().at(0).toString()), QDir::toNativeSeparators(filePath));
}

void TestProjectTreeController::projectTreeControllerRenameEmitsPathRenamedAndUpdatesDisk()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString oldPath = writeFile(root, QStringLiteral("old.apy"), QString::fromUtf8("عدد = 1\n"));
    const QString newPath = root.filePath(QStringLiteral("new.apy"));

    QTreeView tree;
    QFileSystemModel model;
    ProjectTreeController controller(&tree, &model);
    controller.setProjectRoot(root.absolutePath());
    QSignalSpy renamedSpy(&controller, &ProjectTreeController::pathRenamed);

    tree.setCurrentIndex(waitForIndex(model, oldPath));
    auto *renameAction = controller.findChild<QAction *>(QStringLiteral("projectTreeRenameAction"));
    QVERIFY(renameAction != nullptr);

    QTimer::singleShot(0, []() {
        auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog != nullptr);
        dialog->setTextValue(QStringLiteral("new.apy"));
        dialog->accept();
    });
    renameAction->trigger();

    QCOMPARE(renamedSpy.count(), 1);
    const QList<QVariant> args = renamedSpy.takeFirst();
    QCOMPARE(QDir::toNativeSeparators(args.at(0).toString()), QDir::toNativeSeparators(oldPath));
    QCOMPARE(QDir::toNativeSeparators(args.at(1).toString()), QDir::toNativeSeparators(newPath));
    QVERIFY(!QFileInfo::exists(oldPath));
    QVERIFY(QFileInfo::exists(newPath));
}

void TestProjectTreeController::projectTreeControllerDeleteEmitsPathDeletedAndRemovesFromDisk()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("delete-me.apy"), QString::fromUtf8("عدد = 1\n"));

    QTreeView tree;
    QFileSystemModel model;
    ProjectTreeController controller(&tree, &model);
    controller.setDeleteConfirmationCallback([](const QString &, bool) { return true; });
    controller.setProjectRoot(root.absolutePath());
    QSignalSpy deletedSpy(&controller, &ProjectTreeController::pathDeleted);

    tree.setCurrentIndex(waitForIndex(model, filePath));
    auto *deleteAction = controller.findChild<QAction *>(QStringLiteral("projectTreeDeleteAction"));
    QVERIFY(deleteAction != nullptr);

    deleteAction->trigger();

    QCOMPARE(deletedSpy.count(), 1);
    QCOMPARE(QDir::toNativeSeparators(deletedSpy.takeFirst().at(0).toString()), QDir::toNativeSeparators(filePath));
    QVERIFY(!QFileInfo::exists(filePath));
}

void TestProjectTreeController::projectTreeControllerCurrentSelectionPathReturnsAbsolutePath()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QDir root(temp.path());
    const QString filePath = writeFile(root, QStringLiteral("selected.apy"), QString::fromUtf8("عدد = 1\n"));

    QTreeView tree;
    QFileSystemModel model;
    ProjectTreeController controller(&tree, &model);
    controller.setProjectRoot(root.absolutePath());

    QCOMPARE(controller.currentSelectionPath(), QString());

    tree.setCurrentIndex(waitForIndex(model, filePath));

    QCOMPARE(QDir::toNativeSeparators(controller.currentSelectionPath()), QDir::toNativeSeparators(filePath));
}

QTEST_MAIN(TestProjectTreeController)
#include "TestProjectTreeController.moc"
