#include "MainWindow.h"

#include "ApySnippetService.h"
#include "CommandPaletteModel.h"
#include "DocumentFileIO.h"
#include "ProjectFileOperations.h"
#include "ShortcutSettingsModel.h"
#include "WorkbenchTheme.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QKeySequenceEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QSaveFile>
#include <QStatusBar>
#include <QStyle>
#include <QSpinBox>
#include <QTabBar>
#include <QTextBlock>
#include <QTextOption>
#include <QTextStream>
#include <QToolButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <algorithm>
#include <functional>

class ArabicPlaceholderPlainTextEdit final : public QPlainTextEdit
{
public:
    explicit ArabicPlaceholderPlainTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
    }

    void setArabicPlaceholderText(const QString &text)
    {
        placeholder = text;
        setPlaceholderText(QString());
        viewport()->update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPlainTextEdit::paintEvent(event);

        if (!toPlainText().isEmpty() || placeholder.isEmpty()) {
            return;
        }

        QPainter painter(viewport());
        painter.setPen(QColor(145, 155, 160));
        const QRect textRect = viewport()->rect().adjusted(12, 10, -12, 0);
        const int textWidth = fontMetrics().horizontalAdvance(placeholder);
        const int x = qMax(textRect.left(), textRect.right() - textWidth + 1);
        const int y = textRect.top() + fontMetrics().ascent();
        painter.drawText(x, y, placeholder);
    }

private:
    QString placeholder;
};

class MenuPopupFrame final : public QFrame
{
public:
    explicit MenuPopupFrame(QWidget *parent = nullptr)
        : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
    {
    }

    void setOwnerButton(QToolButton *button)
    {
        ownerButton = button;
    }

protected:
    void hideEvent(QHideEvent *event) override
    {
        if (ownerButton) {
            ownerButton->setChecked(false);
            ownerButton->setProperty("active", false);
            ownerButton->style()->unpolish(ownerButton);
            ownerButton->style()->polish(ownerButton);
            ownerButton->update();
        }
        QFrame::hideEvent(event);
    }

private:
    QToolButton *ownerButton = nullptr;
};

static QStringList arabicEditorFontFamilies()
{
    QStringList families = QFontDatabase::families(QFontDatabase::Arabic);
    families.erase(std::remove_if(families.begin(), families.end(), [](const QString &family) {
        return !QFontDatabase::writingSystems(family).contains(QFontDatabase::Arabic)
            || family.contains(QStringLiteral("Cascadia"), Qt::CaseInsensitive)
            || family.contains(QStringLiteral("JetBrains"), Qt::CaseInsensitive)
            || family.compare(QStringLiteral("Consolas"), Qt::CaseInsensitive) == 0;
    }), families.end());
    families.sort(Qt::CaseInsensitive);

    const QStringList preferredFonts = {
        QStringLiteral("Segoe UI"),
        QStringLiteral("Tahoma"),
        QStringLiteral("Arial"),
        QStringLiteral("Courier New"),
        QStringLiteral("Traditional Arabic"),
        QStringLiteral("Simplified Arabic"),
        QStringLiteral("Arabic Typesetting"),
    };

    QStringList orderedFamilies;
    for (const QString &preferred : preferredFonts) {
        if (families.removeOne(preferred)) {
            orderedFamilies.append(preferred);
        }
    }
    orderedFamilies.append(families);
    orderedFamilies.removeDuplicates();

    if (orderedFamilies.isEmpty()) {
        orderedFamilies.append(QStringLiteral("Segoe UI"));
    }
    return orderedFamilies;
}

static QString lineEndingStatusText(DocumentLineEnding lineEnding)
{
    switch (lineEnding) {
    case DocumentLineEnding::Lf:
        return QStringLiteral("LF");
    case DocumentLineEnding::Crlf:
        return QStringLiteral("CRLF");
    case DocumentLineEnding::Mixed:
        return QString::fromUtf8("مختلط");
    case DocumentLineEnding::None:
        return QString::fromUtf8("بدون نهاية");
    }
    return QString::fromUtf8("بدون نهاية");
}

static bool appendWorkspaceTrustAuditEntry(const QString &projectRoot, const QString &eventName, QString *error)
{
    if (error) {
        error->clear();
    }

    const WorkspaceSettingsStore store(projectRoot);
    const QFileInfo auditInfo(store.trustAuditFilePath());
    if (!QDir().mkpath(auditInfo.absolutePath())) {
        if (error) {
            *error = QString::fromUtf8("تم حفظ الثقة، لكن تعذر إنشاء سجل تدقيق الثقة.");
        }
        return false;
    }

    QJsonObject entry;
    entry.insert(QStringLiteral("event"), eventName);
    entry.insert(QStringLiteral("projectRoot"), QDir(projectRoot).absolutePath());
    entry.insert(QStringLiteral("timestampUtc"), QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'")));

    QFile file(auditInfo.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        if (error) {
            *error = QString::fromUtf8("تم حفظ الثقة، لكن تعذر كتابة سجل تدقيق الثقة: %1").arg(file.errorString());
        }
        return false;
    }

    const QByteArray line = QJsonDocument(entry).toJson(QJsonDocument::Compact) + '\n';
    if (file.write(line) != line.size()) {
        if (error) {
            *error = QString::fromUtf8("تم حفظ الثقة، لكن تعذر إكمال كتابة سجل تدقيق الثقة: %1").arg(file.errorString());
        }
        return false;
    }
    return true;
}

static QString languageModeStatusText(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("apy")) {
        return QStringLiteral(".apy");
    }
    if (suffix == QStringLiteral("py")) {
        return QStringLiteral("Python");
    }
    if (suffix == QStringLiteral("md")) {
        return QStringLiteral("Markdown");
    }
    if (suffix == QStringLiteral("txt")) {
        return QString::fromUtf8("نص");
    }
    return QString::fromUtf8("نص عادي");
}

static bool offsetForLspPosition(const QString &text, int targetLine, int targetCharacter, int *offset)
{
    if (!offset || targetLine < 0 || targetCharacter < 0) {
        return false;
    }

    int line = 0;
    int lineStart = 0;
    for (int i = 0; i <= text.size(); ++i) {
        const bool atEnd = i == text.size();
        const bool atNewline = !atEnd && text.at(i) == QLatin1Char('\n');
        if (!atEnd && !atNewline) {
            continue;
        }

        if (line == targetLine) {
            const int lineEnd = atNewline ? i : text.size();
            const int logicalLineEnd = lineEnd > lineStart && text.at(lineEnd - 1) == QLatin1Char('\r')
                ? lineEnd - 1
                : lineEnd;
            if (targetCharacter > logicalLineEnd - lineStart) {
                return false;
            }
            *offset = lineStart + targetCharacter;
            return true;
        }

        ++line;
        lineStart = i + 1;
    }

    return false;
}

static bool textEditToDocumentEdit(const QString &text, const LspTextEdit &lspEdit, DocumentTextEdit *documentEdit)
{
    int startOffset = -1;
    int endOffset = -1;
    if (!offsetForLspPosition(text, lspEdit.startLine, lspEdit.startCharacter, &startOffset)
        || !offsetForLspPosition(text, lspEdit.endLine, lspEdit.endCharacter, &endOffset)
        || endOffset < startOffset) {
        return false;
    }

    if (documentEdit) {
        documentEdit->start = startOffset;
        documentEdit->length = endOffset - startOffset;
        documentEdit->replacement = lspEdit.newText;
    }
    return true;
}

static bool isApySourcePath(const QString &path)
{
    return QFileInfo(path).suffix().compare(QStringLiteral("apy"), Qt::CaseInsensitive) == 0;
}

static QLabel *createStatusIndicator(const QString &objectName, const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(objectName);
    label->setLayoutDirection(Qt::LeftToRight);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    label->setMinimumWidth(72);
    return label;
}

static bool isArabicEditorFontFamily(const QString &family)
{
    const QStringList families = arabicEditorFontFamilies();
    return std::any_of(families.cbegin(), families.cend(), [&family](const QString &candidate) {
        return candidate.compare(family, Qt::CaseInsensitive) == 0;
    });
}

static QString resolvedArabicEditorFontFamily(const QString &configuredFamily)
{
    if (isArabicEditorFontFamily(configuredFamily)) {
        return configuredFamily;
    }
    return arabicEditorFontFamilies().first();
}

static QFont configuredEditorFont(const SettingsStore &settings)
{
    QFont font;
    font.setFamily(resolvedArabicEditorFontFamily(settings.editorFontFamily()));
    font.setPointSize(settings.editorFontSize());
    font.setStyleHint(QFont::Monospace);
    return font;
}

static QIcon lightPlayIcon()
{
    QPixmap pixmap(28, 28);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPolygonF triangle({
        QPointF(10.0, 7.0),
        QPointF(10.0, 21.0),
        QPointF(21.0, 14.0),
    });
    painter.setBrush(QColor(QStringLiteral("#D6E4FF")));
    painter.setPen(QPen(QColor(QStringLiteral("#7BDFF2")), 1.5));
    painter.drawPolygon(triangle);

    return QIcon(pixmap);
}

MainWindow::MainWindow(QWidget *parent)
    : MainWindow(parent, QString())
{
}

MainWindow::MainWindow(QWidget *parent, const QString &settingsPath)
    : QMainWindow(parent),
      settings(settingsPath),
      runtimeOrchestrator(this),
      lspClient(this),
      documentChangePoller(&documentRegistry)
{
    const bool promptForDraftRecovery = settings.hasNonOrderlyShutdown();
    settings.markWorkbenchSessionStarted();

    buildUi();
    configureLanguageServer();
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::outputCleared, this, [this]() {
        showOutputPanel();
        outputTranscript.clear();
        renderOutputTranscript();
    });
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::outputProduced, this, &MainWindow::appendRuntimeOutput);
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::problemDetected, this, &MainWindow::addProblem);
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::problemsRequested, this, &MainWindow::showProblemsPanel);
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::statusChanged, this, &MainWindow::setStatus);
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::runningChanged, this, [this](bool running) {
        if (runAction) runAction->setEnabled(!running);
        if (lintAction) lintAction->setEnabled(!running);
        if (formatAction) formatAction->setEnabled(!running);
        if (cancelRunAction) cancelRunAction->setEnabled(running);
    });
    connect(&runtimeOrchestrator, &RuntimeOrchestrator::reloadRequested, this, [this](const QString &path) {
        if (!editor) return;
        QString error;
        if (editor->openFile(path, &error) && editorTabsController) {
            editorTabsController->syncSessionsInto(workbenchState);
        }
    });
    documentChangePollTimer = new QTimer(this);
    documentChangePollTimer->setInterval(2000);
    connect(documentChangePollTimer, &QTimer::timeout, this, &MainWindow::pollOpenDocumentChanges);
    documentChangePollTimer->start();

    untitledDraftAutosaveTimer = new QTimer(this);
    untitledDraftAutosaveTimer->setObjectName(QStringLiteral("untitledDraftAutosaveTimer"));
    untitledDraftAutosaveTimer->setInterval(5000);
    untitledDraftAutosaveTimer->setSingleShot(true);
    connect(untitledDraftAutosaveTimer, &QTimer::timeout, this, [this]() {
        if (!hasDirtyUntitledDraft()) {
            return;
        }
        saveWorkbenchSession();
        if (hasDirtyUntitledDraft()) {
            untitledDraftAutosaveTimer->start();
        }
    });

    restoreWorkbenchSession(promptForDraftRecovery);
    resize(1280, 820);
    setWindowTitle(QString::fromUtf8("استوديو لسان"));
    setWindowIcon(QIcon(QStringLiteral(":/branding/lisan-logo.png")));
}

bool MainWindow::openPath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return false;
    }
    if (info.isDir()) {
        return loadProject(info.absoluteFilePath());
    }
    if (info.isFile()) {
        return openEditorFile(info.absoluteFilePath());
    }
    return false;
}

QString MainWindow::currentProjectRoot() const
{
    return projectRoot;
}

QString MainWindow::currentEditorPath() const
{
    return workbenchState.currentEditorPath();
}

QString MainWindow::materializeRunnableBuffer(QString *error)
{
    if (error) {
        error->clear();
    }

    if (!editor->currentFilePath().isEmpty()) {
        if (editor->isDirty()) {
            const auto answer = QMessageBox::question(
                this,
                QString::fromUtf8("حفظ قبل المتابعة"),
                QString::fromUtf8("يحتوي الملف الحالي على تغييرات غير محفوظة. احفظه قبل تشغيله؟"),
                QMessageBox::Save | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (answer != QMessageBox::Save) {
                if (error) {
                    *error = QString::fromUtf8("أُلغي التشغيل لأن الملف يحتوي على تغييرات غير محفوظة.");
                }
                return QString();
            }

            if (!confirmHiddenBidiSave()) {
                if (error) {
                    *error = QString::fromUtf8("أُلغي التشغيل لأن الملف يحتوي على محارف اتجاه مخفية.");
                }
                return QString();
            }

            QString saveError;
            if (!editorTabsController || !editorTabsController->saveCurrent(&saveError)) {
                if (error) {
                    *error = saveError;
                }
                return QString();
            }
        }
        return editor->currentFilePath();
    }

    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("تعذر تحديد مجلد التخزين المؤقت.");
        }
        return QString();
    }

    QDir runDirectory(QDir(cacheRoot).filePath(QStringLiteral("lisan-studio/run-buffers")));
    if (!runDirectory.mkpath(QStringLiteral("."))) {
        if (error) {
            *error = QString::fromUtf8("تعذر إنشاء مجلد التشغيل المؤقت.");
        }
        return QString();
    }

    const QString runFilePath = runDirectory.filePath(QStringLiteral("current-buffer.apy"));
    QSaveFile file(runFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return QString();
    }

    file.write(editor->toPlainText().toUtf8());
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return QString();
    }
    return runFilePath;
}

void MainWindow::buildUi()
{
    setLayoutDirection(Qt::RightToLeft);
    registerWorkbenchCommands();

    setStyleSheet(WorkbenchTheme::darkStyleSheet());
    applyThemePreference();

    auto *topShell = new QWidget(this);
    topShell->setObjectName(QStringLiteral("topShell"));
    topShell->setLayoutDirection(Qt::RightToLeft);
    topShell->setFixedHeight(44);

    auto *topMenuRow = new QWidget(topShell);
    topMenuRow->setObjectName(QStringLiteral("topMenuRow"));
    topMenuRow->setLayoutDirection(Qt::RightToLeft);
    topMenuRow->setFixedHeight(44);
    auto *topShellLayout = new QHBoxLayout(topShell);
    topShellLayout->setContentsMargins(0, 0, 0, 0);
    topShellLayout->setSpacing(0);
    topShellLayout->addWidget(topMenuRow);
    auto *menuLayout = new QHBoxLayout(topMenuRow);
    menuLayout->setContentsMargins(12, 4, 12, 4);
    menuLayout->setSpacing(10);
    setMenuWidget(topShell);

    auto *brandBlock = new QWidget(topMenuRow);
    brandBlock->setObjectName(QStringLiteral("brandBlock"));
    brandBlock->setLayoutDirection(Qt::LeftToRight);
    auto *brandLayout = new QHBoxLayout(brandBlock);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(8);

    brandLogoLabel = new QLabel(brandBlock);
    brandLogoLabel->setObjectName(QStringLiteral("brandLogoLabel"));
    brandLogoLabel->setPixmap(QPixmap(QStringLiteral(":/branding/lisan-logo.png")).scaled(
        30,
        30,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));
    brandLogoLabel->setToolTip(QString::fromUtf8("استوديو لسان"));
    auto *brandTextLabel = new QLabel(QStringLiteral("Lisan Studio"), brandBlock);
    brandTextLabel->setObjectName(QStringLiteral("brandTextLabel"));
    brandTextLabel->setToolTip(QString::fromUtf8("استوديو لسان"));
    brandLayout->addWidget(brandLogoLabel);
    brandLayout->addWidget(brandTextLabel);

    QList<MenuPopupFrame *> menuPanels;
    auto makeMenuPanel = [&](const QString &objectName) {
        auto *panel = new MenuPopupFrame(this);
        panel->setObjectName(objectName);
        panel->setProperty("role", "menuPopup");
        panel->setLayoutDirection(Qt::RightToLeft);
        panel->setMinimumWidth(240);
        auto *panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(0, 0, 0, 0);
        panelLayout->setSpacing(0);
        menuPanels.append(panel);
        return panel;
    };

    auto *fileMenu = makeMenuPanel(QStringLiteral("fileMenu"));
    auto *editMenu = makeMenuPanel(QStringLiteral("editMenu"));
    auto *viewMenu = makeMenuPanel(QStringLiteral("viewMenu"));
    auto *toolsMenu = makeMenuPanel(QStringLiteral("toolsMenu"));
    auto *helpMenu = makeMenuPanel(QStringLiteral("helpMenu"));

    auto addMenuButton = [&](const QString &objectName, const QString &label, MenuPopupFrame *menu) {
        auto *button = new QToolButton(topMenuRow);
        button->setObjectName(objectName);
        button->setProperty("role", "topMenu");
        button->setProperty("attachedMenuName", menu->objectName());
        button->setText(label);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setLayoutDirection(Qt::RightToLeft);
        button->setCursor(Qt::PointingHandCursor);
        button->setCheckable(true);
        menu->setOwnerButton(button);
        connect(button, &QToolButton::clicked, this, [button, menu, menuPanels]() {
            if (menu->isVisible()) {
                menu->hide();
                return;
            }
            for (auto *otherMenu : menuPanels) {
                if (otherMenu != menu) {
                    otherMenu->hide();
                }
            }
            const QSize menuSize = menu->sizeHint().expandedTo(QSize(menu->minimumWidth(), 1));
            menu->resize(menuSize);
            const QPoint popupPosition = button->mapToGlobal(QPoint(button->width() - menuSize.width(), button->height()));
            menu->move(popupPosition);
            menu->show();
            menu->raise();
            button->setChecked(true);
            button->setProperty("active", true);
            button->style()->unpolish(button);
            button->style()->polish(button);
            button->update();
        });
        menuLayout->addWidget(button);
        return button;
    };

    addMenuButton(QStringLiteral("topMenuFileButton"), QString::fromUtf8("ملف"), fileMenu);
    addMenuButton(QStringLiteral("topMenuEditButton"), QString::fromUtf8("تحرير"), editMenu);
    addMenuButton(QStringLiteral("topMenuViewButton"), QString::fromUtf8("عرض"), viewMenu);
    addMenuButton(QStringLiteral("topMenuToolsButton"), QString::fromUtf8("أدوات"), toolsMenu);
    addMenuButton(QStringLiteral("topMenuHelpButton"), QString::fromUtf8("مساعدة"), helpMenu);

    auto makeAction = [&](const QIcon &icon, const QString &label, auto slot) {
        auto *action = new QAction(icon, label, this);
        action->setToolTip(label);
        connect(action, &QAction::triggered, this, slot);
        addAction(action);
        return action;
    };

    auto addTopButton = [&](const QString &objectName, QAction *action, const char *role, Qt::ToolButtonStyle buttonStyle) {
        auto *button = new QToolButton(topMenuRow);
        button->setObjectName(objectName);
        button->setProperty("role", role);
        button->setDefaultAction(action);
        button->setToolButtonStyle(buttonStyle);
        button->setLayoutDirection(Qt::RightToLeft);
        if (buttonStyle == Qt::ToolButtonIconOnly) {
            button->setToolTip(action->text());
        }
        menuLayout->addWidget(button);
        return button;
    };

    auto *saveAction = makeAction(style()->standardIcon(QStyle::SP_DialogSaveButton), QString::fromUtf8("حفظ"), &MainWindow::saveFile);
    saveAction->setProperty("commandId", QStringLiteral("file.save"));
    auto *openProjectAction = makeAction(style()->standardIcon(QStyle::SP_DirOpenIcon), QString::fromUtf8("فتح مشروع"), &MainWindow::openFolder);
    openProjectAction->setProperty("commandId", QStringLiteral("project.open"));
    runAction = makeAction(lightPlayIcon(), QString::fromUtf8("تشغيل"), &MainWindow::runCurrentFile);
    runAction->setObjectName(QStringLiteral("runAction"));
    runAction->setProperty("commandId", QStringLiteral("run.currentFile"));
    runAction->setShortcut(QKeySequence(QStringLiteral("F5")));
    runAction->setShortcutContext(Qt::ApplicationShortcut);
    cancelRunAction = makeAction(style()->standardIcon(QStyle::SP_MediaStop), QString::fromUtf8("إيقاف"), &MainWindow::cancelRuntimeProcess);
    cancelRunAction->setObjectName(QStringLiteral("cancelRunAction"));
    cancelRunAction->setProperty("commandId", QStringLiteral("run.stop"));
    cancelRunAction->setShortcut(QKeySequence(QStringLiteral("Shift+F5")));
    cancelRunAction->setShortcutContext(Qt::ApplicationShortcut);
    cancelRunAction->setEnabled(false);
    lintAction = makeAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), QString::fromUtf8("فحص"), &MainWindow::lintCurrentFile);
    lintAction->setObjectName(QStringLiteral("lintAction"));
    lintAction->setProperty("commandId", QStringLiteral("run.lintCurrentFile"));
    formatAction = makeAction(style()->standardIcon(QStyle::SP_BrowserReload), QString::fromUtf8("تنسيق"), &MainWindow::formatCurrentFile);
    formatAction->setObjectName(QStringLiteral("formatAction"));
    formatAction->setProperty("commandId", QStringLiteral("run.formatCurrentFile"));
    commandPaletteAction = makeAction(style()->standardIcon(QStyle::SP_FileDialogListView), QString::fromUtf8("لوحة الأوامر"), &MainWindow::openCommandPalette);
    commandPaletteAction->setObjectName(QStringLiteral("commandPaletteAction"));
    commandPaletteAction->setProperty("commandId", QStringLiteral("system.commandPalette"));
    commandPaletteAction->setShortcuts({QKeySequence(QStringLiteral("Ctrl+Shift+P"))});
    auto *settingsAction = makeAction(QIcon(), QString::fromUtf8("الإعدادات"), &MainWindow::openSettings);
    settingsAction->setObjectName(QStringLiteral("settingsAction"));
    settingsAction->setProperty("commandId", QStringLiteral("system.settings"));

    addTopButton(QStringLiteral("topRunButton"), runAction, "primaryAction", Qt::ToolButtonTextBesideIcon);

    auto addTextOnlyMenuAction = [this](
        QFrame *menu,
        const QString &text,
        const QKeySequence &shortcut = QKeySequence(),
        const QString &commandId = QString()) {
        auto *action = new QAction(text, this);
        action->setText(text);
        action->setIcon(QIcon());
        action->setIconVisibleInMenu(false);
        if (!commandId.isEmpty()) {
            action->setProperty("commandId", commandId);
        }
        if (!shortcut.isEmpty()) {
            action->setShortcut(shortcut);
            action->setShortcutContext(Qt::ApplicationShortcut);
            addAction(action);
        }
        auto *row = new QPushButton(menu);
        row->setProperty("role", "menuRow");
        row->setIcon(QIcon());
        row->setText(text);
        row->setLayoutDirection(Qt::RightToLeft);
        row->setCursor(Qt::PointingHandCursor);
        row->setMinimumWidth(menu->minimumWidth());
        row->setFlat(true);
        if (!commandId.isEmpty()) {
            row->setProperty("commandId", commandId);
        }
        if (auto *menuLayout = qobject_cast<QVBoxLayout *>(menu->layout())) {
            menuLayout->addWidget(row);
        }
        connect(row, &QPushButton::clicked, action, &QAction::trigger);
        connect(row, &QPushButton::clicked, menu, &QFrame::hide);
        return action;
    };

    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("ملف جديد"), QKeySequence::New, QStringLiteral("file.new")), &QAction::triggered, this, &MainWindow::newFile);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("فتح ملف"), QKeySequence::Open, QStringLiteral("file.open")), &QAction::triggered, this, &MainWindow::openFile);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("فتح مشروع"), QKeySequence(), QStringLiteral("project.open")), &QAction::triggered, this, &MainWindow::openFolder);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("حفظ"), QKeySequence::Save, QStringLiteral("file.save")), &QAction::triggered, this, &MainWindow::saveFile);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("حفظ باسم"), QKeySequence::SaveAs, QStringLiteral("file.saveAs")), &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("تراجع"), QKeySequence::Undo), &QAction::triggered, this, [this]() {
        if (editor) {
            editor->undo();
        }
    });
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إعادة"), QKeySequence::Redo), &QAction::triggered, this, [this]() {
        if (editor) {
            editor->redo();
        }
    });
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("بحث واستبدال"), QKeySequence::Find, QStringLiteral("editor.findInFile")), &QAction::triggered, this, &MainWindow::openInFileFind);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إضافة مؤشر أعلى"), QKeySequence(QStringLiteral("Ctrl+Alt+Up")), QStringLiteral("cursor.addAbove")), &QAction::triggered, this, &MainWindow::addCursorAboveAction);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إضافة مؤشر أسفل"), QKeySequence(QStringLiteral("Ctrl+Alt+Down")), QStringLiteral("cursor.addBelow")), &QAction::triggered, this, &MainWindow::addCursorBelowAction);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إضافة مؤشر عند المطابقة التالية"), QKeySequence(QStringLiteral("Ctrl+D")), QStringLiteral("cursor.addAtNextMatch")), &QAction::triggered, this, &MainWindow::addCursorAtNextMatchAction);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("تحديد كل المطابقات"), QKeySequence(QStringLiteral("Ctrl+Shift+L")), QStringLiteral("cursor.selectAllMatches")), &QAction::triggered, this, &MainWindow::selectAllCursorMatchesAction);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("انتقال إلى التعريف"), QKeySequence(QStringLiteral("F12")), QStringLiteral("lsp.goToDefinition")), &QAction::triggered, this, [this]() {
        if (!editor) {
            return;
        }
        const QTextCursor cursor = editor->textCursor();
        requestLanguageServerDefinition(cursor.blockNumber(), cursor.position() - cursor.block().position());
    });
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إيجاد المراجع"), QKeySequence(QStringLiteral("Shift+F12")), QStringLiteral("lsp.findReferences")), &QAction::triggered, this, &MainWindow::requestLanguageServerReferences);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إعادة تسمية الرمز"), QKeySequence(QStringLiteral("F2")), QStringLiteral("lsp.renameSymbol")), &QAction::triggered, this, &MainWindow::requestLanguageServerRename);
    // Esc is handled directly by EditorSurface so the app-level action does not steal normal editor cancellation.
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("الرجوع إلى مؤشر واحد"), QKeySequence(), QStringLiteral("cursor.collapseToSingle")), &QAction::triggered, this, &MainWindow::collapseToSingleCursorAction);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إدراج اطبع"), QKeySequence(), QStringLiteral("snippet.insertPrint")), &QAction::triggered, this, &MainWindow::insertPrintSnippet);
    commandPaletteAction->setIconVisibleInMenu(false);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("لوحة الأوامر"), QKeySequence(), QStringLiteral("system.commandPalette")), &QAction::triggered, this, &MainWindow::openCommandPalette);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("إظهار المسافات"), QKeySequence(), QStringLiteral("editor.toggleVisibleWhitespace")), &QAction::triggered, this, &MainWindow::toggleVisibleWhitespace);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("حذف فراغات آخر السطر عند الحفظ"), QKeySequence(), QStringLiteral("editor.toggleTrimTrailingWhitespace")), &QAction::triggered, this, &MainWindow::toggleTrimTrailingWhitespace);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("نسخ الإخراج"), QKeySequence(), QStringLiteral("output.copy")), &QAction::triggered, this, &MainWindow::copyOutputPanel);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("مسح الإخراج"), QKeySequence(), QStringLiteral("output.clear")), &QAction::triggered, this, &MainWindow::clearOutputPanel);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("حفظ الإخراج باسم"), QKeySequence(), QStringLiteral("output.saveAs")), &QAction::triggered, this, &MainWindow::saveOutputPanel);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("كل الإخراج"), QKeySequence(), QStringLiteral("output.filter.all")), &QAction::triggered, this, &MainWindow::showAllOutput);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("إخراج stdout فقط"), QKeySequence(), QStringLiteral("output.filter.stdout")), &QAction::triggered, this, &MainWindow::showOnlyStdoutOutput);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("إخراج stderr فقط"), QKeySequence(), QStringLiteral("output.filter.stderr")), &QAction::triggered, this, &MainWindow::showOnlyStderrOutput);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("رسائل النظام فقط"), QKeySequence(), QStringLiteral("output.filter.system")), &QAction::triggered, this, &MainWindow::showOnlySystemOutput);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("فتح طرفية PowerShell"), QKeySequence(), QStringLiteral("terminal.openPowerShell")), &QAction::triggered, this, &MainWindow::openPowerShellTerminal);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("فحص"), QKeySequence(), QStringLiteral("run.lintCurrentFile")), &QAction::triggered, this, &MainWindow::lintCurrentFile);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("تنسيق"), QKeySequence(), QStringLiteral("run.formatCurrentFile")), &QAction::triggered, this, &MainWindow::formatCurrentFile);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("الثقة بمساحة العمل"), QKeySequence(), QStringLiteral("workspace.trust")), &QAction::triggered, this, &MainWindow::trustCurrentWorkspace);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("إلغاء الثقة بمساحة العمل"), QKeySequence(), QStringLiteral("workspace.untrust")), &QAction::triggered, this, &MainWindow::untrustCurrentWorkspace);
    settingsAction->setIconVisibleInMenu(false);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("الإعدادات"), QKeySequence(), QStringLiteral("system.settings")), &QAction::triggered, this, &MainWindow::openSettings);
    connect(addTextOnlyMenuAction(helpMenu, QString::fromUtf8("عن استوديو لسان")), &QAction::triggered, this, [this]() {
        QMessageBox::information(this, QString::fromUtf8("عن استوديو لسان"), QString::fromUtf8("استوديو لسان\nبيئة عربية أصلية لملفات .apy"));
    });

    commandBox = new QLineEdit(this);
    commandBox->setObjectName(QStringLiteral("commandBox"));
    commandBox->setProperty("commandId", QStringLiteral("search.project"));
    commandBox->setPlaceholderText(QString::fromUtf8("ابحث في الأوامر والملفات..."));
    commandBox->setLayoutDirection(Qt::RightToLeft);
    commandBox->setFixedWidth(440);
    connect(commandBox, &QLineEdit::returnPressed, this, &MainWindow::findInProject);

    projectReplaceInput = new QLineEdit(this);
    projectReplaceInput->setObjectName(QStringLiteral("projectReplaceInput"));
    projectReplaceInput->setProperty("commandId", QStringLiteral("search.replacePreview"));
    projectReplaceInput->setPlaceholderText(QString::fromUtf8("استبدال بـ..."));
    projectReplaceInput->setLayoutDirection(Qt::RightToLeft);
    projectReplaceInput->setFixedWidth(180);

    projectReplacePreviewButton = new QPushButton(QString::fromUtf8("معاينة"), this);
    projectReplacePreviewButton->setObjectName(QStringLiteral("projectReplacePreviewButton"));
    projectReplacePreviewButton->setProperty("commandId", QStringLiteral("search.replacePreview"));
    connect(projectReplacePreviewButton, &QPushButton::clicked, this, &MainWindow::previewProjectReplace);

    projectReplaceApplyButton = new QPushButton(QString::fromUtf8("تطبيق"), this);
    projectReplaceApplyButton->setObjectName(QStringLiteral("projectReplaceApplyButton"));
    projectReplaceApplyButton->setProperty("commandId", QStringLiteral("search.replaceApplyAccepted"));
    connect(projectReplaceApplyButton, &QPushButton::clicked, this, &MainWindow::applyAcceptedProjectReplaceRows);

    menuLayout->addStretch(1);
    menuLayout->addWidget(commandBox);
    menuLayout->addWidget(projectReplaceInput);
    menuLayout->addWidget(projectReplacePreviewButton);
    menuLayout->addWidget(projectReplaceApplyButton);
    menuLayout->addStretch(1);
    menuLayout->addWidget(brandBlock);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setLayoutDirection(Qt::RightToLeft);

    fileSystemModel = new QFileSystemModel(this);
    fileSystemModel->setNameFilters({QStringLiteral("*.apy"), QStringLiteral("*.py"), QStringLiteral("*.md"), QStringLiteral("*.txt")});
    fileSystemModel->setNameFilterDisables(false);

    projectTree = new QTreeView(splitter);
    projectTree->setObjectName(QStringLiteral("projectTree"));
    projectTreeController = std::make_unique<ProjectTreeController>(projectTree, fileSystemModel, this);
    projectTreeController->setDeleteConfirmationCallback([this](const QString &path, bool isDirectory) {
        QMessageBox box(this);
        box.setLayoutDirection(Qt::RightToLeft);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(QString::fromUtf8("تأكيد الحذف"));
        box.setText(isDirectory
            ? QString::fromUtf8("هل تريد حذف هذا المجلد وكل محتوياته؟")
            : QString::fromUtf8("هل تريد حذف هذا الملف؟"));
        box.setInformativeText(QDir::toNativeSeparators(path));
        auto *deleteButton = box.addButton(QString::fromUtf8("حذف"), QMessageBox::AcceptRole);
        box.addButton(QString::fromUtf8("إلغاء"), QMessageBox::RejectRole);
        box.setDefaultButton(qobject_cast<QPushButton *>(box.buttons().last()));
        box.exec();
        return box.clickedButton() == deleteButton;
    });
    connect(projectTreeController.get(), &ProjectTreeController::openPathRequested, this, [this](const QString &path) {
        if (editor && editor->isDirty() && !confirmUnsavedDocuments(UnsavedChangesOperation::OpenFile)) {
            return;
        }

        QString error;
        if (!editorTabsController || !editorTabsController->openFile(path, &error)) {
            QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
            return;
        }
        settings.addRecentFile(QFileInfo(path).absoluteFilePath());
        setStatus(QString::fromUtf8("فتح: %1").arg(QFileInfo(path).fileName()));
    });
    connect(projectTreeController.get(), &ProjectTreeController::revealRequested, this, [](const QString &path) {
        QProcess::startDetached(QStringLiteral("explorer.exe"), ProjectFileOperations::explorerRevealArguments(path));
    });
    connect(projectTreeController.get(), &ProjectTreeController::pathDeleted, this, &MainWindow::clearEditorsForDeletedPath);
    connect(projectTreeController.get(), &ProjectTreeController::pathRenamed, this, [this](const QString &oldPath, const QString &newPath) {
        if (QFileInfo(newPath).isFile() && QFileInfo(currentEditorPath()).absoluteFilePath() == QFileInfo(oldPath).absoluteFilePath()) {
            openEditorFile(newPath);
        }
    });
    connect(projectTreeController.get(), &ProjectTreeController::statusMessage, this, &MainWindow::setStatus);
    connect(projectTreeController.get(), &ProjectTreeController::errorMessage, this, [this](const QString &title, const QString &body) {
        QMessageBox::warning(this, title, body);
    });

    auto *editorColumn = new QWidget(splitter);
    editorColumn->setObjectName(QStringLiteral("editorColumn"));
    editorColumn->setLayoutDirection(Qt::RightToLeft);
    auto *editorColumnLayout = new QVBoxLayout(editorColumn);
    editorColumnLayout->setContentsMargins(0, 0, 0, 0);
    editorColumnLayout->setSpacing(6);

    auto *breadcrumbBar = new QWidget(editorColumn);
    breadcrumbBar->setObjectName(QStringLiteral("breadcrumbBar"));
    breadcrumbBar->setLayoutDirection(Qt::RightToLeft);
    auto *breadcrumbLayout = new QHBoxLayout(breadcrumbBar);
    breadcrumbLayout->setContentsMargins(8, 6, 8, 0);
    breadcrumbLayout->setSpacing(8);
    breadcrumbPathLabel = new QLabel(QString::fromUtf8("ملف جديد"), breadcrumbBar);
    breadcrumbPathLabel->setObjectName(QStringLiteral("breadcrumbPathLabel"));
    breadcrumbPathLabel->setLayoutDirection(Qt::LeftToRight);
    breadcrumbPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    breadcrumbPathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    breadcrumbSymbolLabel = new QLabel(QString::fromUtf8("الرموز: لاحقا"), breadcrumbBar);
    breadcrumbSymbolLabel->setObjectName(QStringLiteral("breadcrumbSymbolLabel"));
    breadcrumbSymbolLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    breadcrumbLayout->addWidget(breadcrumbSymbolLabel);
    breadcrumbLayout->addWidget(breadcrumbPathLabel, 1);
    editorColumnLayout->addWidget(breadcrumbBar);

    inFileFindPanel = new QWidget(editorColumn);
    inFileFindPanel->setObjectName(QStringLiteral("inFileFindPanel"));
    inFileFindPanel->setLayoutDirection(Qt::RightToLeft);
    inFileFindPanel->hide();
    auto *findLayout = new QHBoxLayout(inFileFindPanel);
    findLayout->setContentsMargins(8, 6, 8, 0);
    findLayout->setSpacing(6);

    inFileFindInput = new QLineEdit(inFileFindPanel);
    inFileFindInput->setObjectName(QStringLiteral("inFileFindInput"));
    inFileFindInput->setPlaceholderText(QString::fromUtf8("بحث في الملف"));
    inFileFindInput->setLayoutDirection(Qt::RightToLeft);

    inFileReplaceInput = new QLineEdit(inFileFindPanel);
    inFileReplaceInput->setObjectName(QStringLiteral("inFileReplaceInput"));
    inFileReplaceInput->setPlaceholderText(QString::fromUtf8("استبدال"));
    inFileReplaceInput->setLayoutDirection(Qt::RightToLeft);

    auto *previousFindButton = new QPushButton(QString::fromUtf8("السابق"), inFileFindPanel);
    previousFindButton->setObjectName(QStringLiteral("inFileFindPreviousButton"));
    auto *nextFindButton = new QPushButton(QString::fromUtf8("التالي"), inFileFindPanel);
    nextFindButton->setObjectName(QStringLiteral("inFileFindNextButton"));
    auto *replaceButton = new QPushButton(QString::fromUtf8("استبدال"), inFileFindPanel);
    replaceButton->setObjectName(QStringLiteral("inFileReplaceButton"));
    auto *replaceAllButton = new QPushButton(QString::fromUtf8("استبدال الكل"), inFileFindPanel);
    replaceAllButton->setObjectName(QStringLiteral("inFileReplaceAllButton"));
    inFileFindStatusLabel = new QLabel(QStringLiteral("0"), inFileFindPanel);
    inFileFindStatusLabel->setObjectName(QStringLiteral("inFileFindStatusLabel"));
    inFileFindStatusLabel->setMinimumWidth(48);
    inFileFindStatusLabel->setAlignment(Qt::AlignCenter);

    findLayout->addWidget(inFileFindInput, 2);
    findLayout->addWidget(previousFindButton);
    findLayout->addWidget(nextFindButton);
    findLayout->addWidget(inFileReplaceInput, 2);
    findLayout->addWidget(replaceButton);
    findLayout->addWidget(replaceAllButton);
    findLayout->addWidget(inFileFindStatusLabel);

    connect(inFileFindInput, &QLineEdit::textChanged, this, &MainWindow::updateInFileFindMatches);
    connect(previousFindButton, &QPushButton::clicked, this, &MainWindow::selectPreviousInFileMatch);
    connect(nextFindButton, &QPushButton::clicked, this, &MainWindow::selectNextInFileMatch);
    connect(replaceButton, &QPushButton::clicked, this, &MainWindow::replaceCurrentInFileMatch);
    connect(replaceAllButton, &QPushButton::clicked, this, &MainWindow::replaceAllInFileMatches);
    editorColumnLayout->addWidget(inFileFindPanel);

    editorTabs = new QTabWidget(editorColumn);
    editorTabs->setObjectName(QStringLiteral("editorTabs"));
    editorTabs->setDocumentMode(true);
    editorTabs->setTabsClosable(true);
    editorTabs->setLayoutDirection(Qt::RightToLeft);
    editorTabsController = std::make_unique<EditorTabsController>(editorTabs, documentRegistry, workbenchState, this);
    editorTabsController->applyFont(configuredEditorFont(settings));
    editorTabsController->applyWorkspaceSettings(workspaceSettings);
    connect(editorTabsController.get(), &EditorTabsController::currentEditorChanged, this, [this](EditorSurface *surface) {
        editor = surface;
        if (surface && !surface->property("multiCursorSignalsConnected").toBool()) {
            surface->setProperty("multiCursorSignalsConnected", true);
            connect(surface, &EditorSurface::cursorSoftCapReached, this, [this](int totalCursors) {
                if (multiCursorSoftCapNoticeShown) {
                    return;
                }
                multiCursorSoftCapNoticeShown = true;
                setStatus(QString::fromUtf8("تم تجاوز %1 مؤشر — قد يتأثر الأداء").arg(totalCursors));
            });
            connect(surface, &EditorSurface::cursorCountChanged, this, [this](int totalCursors) {
                if (totalCursors < EditorSurface::kSoftCursorCap) {
                    multiCursorSoftCapNoticeShown = false;
                }
            });
        }
        if (surface && !surface->property("untitledDraftAutosaveConnected").toBool()) {
            surface->setProperty("untitledDraftAutosaveConnected", true);
            connect(surface->document(), &QTextDocument::contentsChanged, this, &MainWindow::scheduleUntitledDraftAutosave);
        }
        if (surface && !surface->property("lspDocumentSyncConnected").toBool()) {
            surface->setProperty("lspDocumentSyncConnected", true);
            connect(surface->document(), &QTextDocument::contentsChanged, this, [this]() {
                syncCurrentEditorToLanguageServer(false);
            });
            connect(surface, &EditorSurface::completionRequested, this, [this, surface](int line, int character) {
                if (surface == editor) {
                    requestLanguageServerCompletion(line, character);
                }
            });
            connect(surface, &EditorSurface::hoverRequested, this, [this, surface](int line, int character, QPoint viewportPosition) {
                if (surface == editor) {
                    requestLanguageServerHover(line, character, viewportPosition);
                }
            });
            connect(surface, &EditorSurface::definitionRequested, this, [this, surface](int line, int character) {
                if (surface == editor) {
                    requestLanguageServerDefinition(line, character);
                }
            });
        }
        if (!surface || surface->totalCursorCount() < EditorSurface::kSoftCursorCap) {
            multiCursorSoftCapNoticeShown = false;
        }
        syncCurrentEditorToLanguageServer(true);
        refreshCurrentEditorUi(true);
    });
    connect(editorTabsController.get(), &EditorTabsController::pathChanged, this, [this](DocumentId, const QString &) {
        syncCurrentEditorToLanguageServer(true);
        refreshCurrentEditorUi(false);
    });
    connect(editorTabsController.get(), &EditorTabsController::dirtyStateChanged, this, [this](DocumentId, bool) {
        refreshCurrentEditorUi(true);
        saveWorkbenchSession();
    });
    connect(editorTabsController.get(), &EditorTabsController::tabClosed, this, [this](DocumentId) {
        refreshEditorProblems();
        saveWorkbenchSession();
    });
    connect(editorTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        requestCloseEditorTab(index);
    });
    editorTabsController->createUntitled(QString::fromUtf8("ملف جديد"));
    editorColumnLayout->addWidget(editorTabs, 1);

    splitter->addWidget(projectTree);
    splitter->addWidget(editorColumn);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    bottomPanelTabs = new QTabWidget(this);
    bottomPanelTabs->setObjectName(QStringLiteral("bottomPanelTabs"));
    bottomPanelTabs->setLayoutDirection(Qt::RightToLeft);

    auto *arabicOutputPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    outputPanel = arabicOutputPanel;
    outputPanel->setObjectName(QStringLiteral("outputPanel"));
    outputPanel->setReadOnly(true);
    outputPanel->setLayoutDirection(Qt::RightToLeft);
    QTextOption outputOption = outputPanel->document()->defaultTextOption();
    outputOption.setTextDirection(Qt::RightToLeft);
    outputOption.setAlignment(Qt::AlignRight);
    outputPanel->document()->setDefaultTextOption(outputOption);
    outputPanel->setMinimumHeight(160);
    outputPanel->viewport()->installEventFilter(this);
    outputPanel->setToolTip(QString::fromUtf8("انقر نقرا مزدوجا على مسار ملف لفتحه."));
    arabicOutputPanel->setArabicPlaceholderText(QString::fromUtf8("المخرجات ستظهر هنا"));

    auto *arabicTerminalPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    terminalPanel = arabicTerminalPanel;
    terminalPanel->setObjectName(QStringLiteral("terminalPanel"));
    terminalPanel->setReadOnly(true);
    terminalPanel->setLayoutDirection(Qt::RightToLeft);
    arabicTerminalPanel->setArabicPlaceholderText(QString::fromUtf8("الطرفية ستظهر هنا"));

    problemsPanel = new QListWidget(bottomPanelTabs);
    problemsPanel->setObjectName(QStringLiteral("problemsPanel"));
    problemsPanel->setLayoutDirection(Qt::RightToLeft);
    problemsPanel->setWordWrap(true);
    problemsPanel->setUniformItemSizes(false);
    problemsPanel->setToolTip(QString::fromUtf8("تحذيرات الاتجاه وأخطاء التشغيل القابلة للفتح."));
    connect(problemsPanel, &QListWidget::itemClicked, this, &MainWindow::openProblemResult);
    connect(problemsPanel, &QListWidget::itemActivated, this, &MainWindow::openProblemResult);
    connect(problemsPanel, &QListWidget::itemDoubleClicked, this, &MainWindow::openProblemResult);

    searchResultsPanel = new QListWidget(bottomPanelTabs);
    searchResultsPanel->setObjectName(QStringLiteral("searchResultsPanel"));
    searchResultsPanel->setLayoutDirection(Qt::RightToLeft);
    searchResultsPanel->setWordWrap(true);
    searchResultsPanel->setUniformItemSizes(false);
    searchResultsPanel->setToolTip(QString::fromUtf8("نتائج البحث في المشروع. اضغط Enter أو انقر مرتين للفتح."));
    connect(searchResultsPanel, &QListWidget::itemClicked, this, &MainWindow::openSearchResult);
    connect(searchResultsPanel, &QListWidget::itemActivated, this, &MainWindow::openSearchResult);
    connect(searchResultsPanel, &QListWidget::itemDoubleClicked, this, &MainWindow::openSearchResult);

    referencesPanel = new QListWidget(bottomPanelTabs);
    referencesPanel->setObjectName(QStringLiteral("referencesPanel"));
    referencesPanel->setLayoutDirection(Qt::RightToLeft);
    referencesPanel->setWordWrap(true);
    referencesPanel->setUniformItemSizes(false);
    referencesPanel->setToolTip(QString::fromUtf8("مراجع الرمز من خادم اللغة. اضغط Enter أو انقر مرتين للفتح."));
    connect(referencesPanel, &QListWidget::itemClicked, this, &MainWindow::openReferenceResult);
    connect(referencesPanel, &QListWidget::itemActivated, this, &MainWindow::openReferenceResult);
    connect(referencesPanel, &QListWidget::itemDoubleClicked, this, &MainWindow::openReferenceResult);

    auto *arabicDebugPanel = new ArabicPlaceholderPlainTextEdit(bottomPanelTabs);
    debugPanel = arabicDebugPanel;
    debugPanel->setObjectName(QStringLiteral("debugPanel"));
    debugPanel->setReadOnly(true);
    debugPanel->setLayoutDirection(Qt::RightToLeft);
    arabicDebugPanel->setArabicPlaceholderText(QString::fromUtf8("بيانات التصحيح ستظهر هنا"));

    bottomPanelTabs->addTab(terminalPanel, QString::fromUtf8("الطرفية"));
    bottomPanelTabs->addTab(outputPanel, QString::fromUtf8("الإخراج"));
    bottomPanelTabs->addTab(problemsPanel, QString::fromUtf8("المشاكل"));
    bottomPanelTabs->addTab(searchResultsPanel, QString::fromUtf8("نتائج البحث"));
    bottomPanelTabs->addTab(referencesPanel, QString::fromUtf8("المراجع"));
    bottomPanelTabs->addTab(debugPanel, QString::fromUtf8("التصحيح"));
    bottomPanels = std::make_unique<BottomPanelController>(
        bottomPanelTabs,
        outputPanel,
        terminalPanel,
        problemsPanel,
        searchResultsPanel,
        referencesPanel,
        debugPanel);

    outputDock = new QDockWidget(QString::fromUtf8("اللوحة السفلية"), this);
    outputDock->setObjectName(QStringLiteral("outputDock"));
    outputDock->setWidget(bottomPanelTabs);
    outputDock->setMinimumHeight(180);
    outputDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, outputDock);
    resizeDocks({outputDock}, {190}, Qt::Vertical);

    statusLabel = new QLabel(QString::fromUtf8("جاهز"), this);
    statusLabel->setObjectName(QStringLiteral("statusLabel"));
    statusBar()->addPermanentWidget(statusLabel, 1);
    statusEncodingLabel = createStatusIndicator(QStringLiteral("statusEncodingLabel"), QStringLiteral("UTF-8"), this);
    statusLineEndingLabel = createStatusIndicator(QStringLiteral("statusLineEndingLabel"), QString::fromUtf8("بدون نهاية"), this);
    statusIndentationLabel = createStatusIndicator(QStringLiteral("statusIndentationLabel"), QString::fromUtf8("مسافات: 4"), this);
    statusLanguageModeLabel = createStatusIndicator(QStringLiteral("statusLanguageModeLabel"), QString::fromUtf8("نص عادي"), this);
    statusRuntimeLabel = createStatusIndicator(QStringLiteral("statusRuntimeLabel"), QString::fromUtf8("التشغيل: جاهز"), this);
    statusGitLabel = createStatusIndicator(QStringLiteral("statusGitLabel"), QStringLiteral("Git: --"), this);
    statusBar()->addPermanentWidget(statusEncodingLabel);
    statusBar()->addPermanentWidget(statusLineEndingLabel);
    statusBar()->addPermanentWidget(statusIndentationLabel);
    statusBar()->addPermanentWidget(statusLanguageModeLabel);
    statusBar()->addPermanentWidget(statusRuntimeLabel);
    statusBar()->addPermanentWidget(statusGitLabel);
    applyShortcutSettings();
    updateStatusIndicators();

    commandBox->setProperty("focusOrderIndex", 10);
    projectTree->setProperty("focusOrderIndex", 20);
    editorTabs->setProperty("focusOrderIndex", 30);
    if (editor) {
        editor->setProperty("focusOrderIndex", 40);
    }
    bottomPanelTabs->setProperty("focusOrderIndex", 50);

    commandBox->setFocusPolicy(Qt::StrongFocus);
    projectTree->setFocusPolicy(Qt::StrongFocus);
    editorTabs->setFocusPolicy(Qt::StrongFocus);
    if (editor) {
        editor->setFocusPolicy(Qt::StrongFocus);
    }
    bottomPanelTabs->setFocusPolicy(Qt::StrongFocus);

    setTabOrder(commandBox, projectTree);
    setTabOrder(projectTree, editorTabs);
    if (editor) {
        setTabOrder(editorTabs, editor);
        setTabOrder(editor, bottomPanelTabs);
    } else {
        setTabOrder(editorTabs, bottomPanelTabs);
    }
}

void MainWindow::newFile()
{
    if (!confirmSaveIfDirty()) {
        return;
    }
    if (editorTabsController) {
        editorTabsController->createUntitled(QString::fromUtf8("ملف جديد"));
    }
    outputPanel->clear();
    refreshEditorProblems();
    setStatus(QString::fromUtf8("ملف جديد"));
}

void MainWindow::openFile()
{
    if (!confirmSaveIfDirty()) {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("فتح ملف"),
        projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
        QString::fromUtf8("ملفات الكود (*.apy *.py *.md *.txt)"));
    if (!path.isEmpty()) {
        openEditorFile(path);
    }
}

void MainWindow::saveFile()
{
    if (editor->currentFilePath().isEmpty()) {
        saveFileAs();
        return;
    }

    if (!confirmHiddenBidiSave()) {
        setStatus(QString::fromUtf8("أُلغي الحفظ"));
        return;
    }

    QString error;
    if (!editorTabsController || !editorTabsController->saveCurrent(&error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر الحفظ"), error);
        return;
    }
    notifyLanguageServerOfSave();
    setStatus(QString::fromUtf8("تم الحفظ"));
}

void MainWindow::saveFileAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("حفظ باسم"),
        projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
        QString::fromUtf8("ملفات لغة الثعبان (*.apy);;كل الملفات (*)"));
    if (path.isEmpty()) {
        return;
    }

    if (!confirmHiddenBidiSave()) {
        setStatus(QString::fromUtf8("أُلغي الحفظ"));
        return;
    }

    QString error;
    if (!editorTabsController || !editorTabsController->saveCurrentAs(path, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر الحفظ"), error);
        return;
    }
    notifyLanguageServerOfSave();
    setStatus(QString::fromUtf8("تم الحفظ"));
}

void MainWindow::openFolder()
{
    const QString path = QFileDialog::getExistingDirectory(this, QString::fromUtf8("فتح مشروع"), QDir::homePath());
    if (!path.isEmpty()) {
        loadProject(path);
    }
}

void MainWindow::runCurrentFile()
{
    runRuntimeAction(RuntimeAction::Run, QString::fromUtf8("تشغيل"));
}

void MainWindow::lintCurrentFile()
{
    runRuntimeAction(RuntimeAction::Lint, QString::fromUtf8("فحص"));
}

void MainWindow::formatCurrentFile()
{
    runRuntimeAction(RuntimeAction::Format, QString::fromUtf8("تنسيق"), true);
}

void MainWindow::rerunLastRuntimeAction()
{
    showOutputPanel();
    runtimeOrchestrator.rerunLast();
}

void MainWindow::cancelRuntimeProcess()
{
    runtimeOrchestrator.cancel();
}

void MainWindow::openCommandPalette()
{
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("commandPaletteDialog"));
    dialog.setWindowTitle(QString::fromUtf8("لوحة الأوامر"));
    dialog.setLayoutDirection(Qt::RightToLeft);
    dialog.resize(640, 420);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *input = new QLineEdit(&dialog);
    input->setObjectName(QStringLiteral("commandPaletteInput"));
    input->setPlaceholderText(QString::fromUtf8("اكتب اسم الأمر..."));
    input->setLayoutDirection(Qt::RightToLeft);

    auto *commands = new QListWidget(&dialog);
    commands->setObjectName(QStringLiteral("commandPaletteResults"));
    commands->setLayoutDirection(Qt::RightToLeft);
    commands->setUniformItemSizes(false);

    ShortcutSettingsModel shortcutSettings;
    const QJsonObject shortcutJson = settings.shortcutSettingsJson();
    const bool hasShortcutSettings = !shortcutJson.isEmpty() && shortcutSettings.loadJson(shortcutJson, commandRegistry);

    auto addCommandRow = [&](const CommandPaletteRow &command) {
        auto *item = new QListWidgetItem(commands);
        item->setData(Qt::UserRole, command.id);
        item->setData(Qt::UserRole + 1, command.title);
        item->setData(Qt::UserRole + 2, command.shortcutText);
        item->setData(Qt::UserRole + 3, command.keywords);
        item->setSizeHint(QSize(0, 44));

        auto *row = new QWidget(commands);
        row->setLayoutDirection(Qt::LeftToRight);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 6, 12, 6);
        rowLayout->setSpacing(12);

        auto *shortcut = new QLabel(command.shortcutText, row);
        shortcut->setObjectName(QStringLiteral("commandPaletteShortcutLabel"));
        shortcut->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        shortcut->setMinimumWidth(120);
        shortcut->setStyleSheet(QStringLiteral("color: #A7B0BE; font-family: 'Cascadia Code', 'Consolas';"));

        auto *title = new QLabel(command.title, row);
        title->setObjectName(QStringLiteral("commandPaletteTitleLabel"));
        title->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        title->setLayoutDirection(Qt::RightToLeft);
        title->setStyleSheet(QStringLiteral("color: #E8ECF2; font-weight: 600;"));

        rowLayout->addWidget(shortcut);
        rowLayout->addStretch(1);
        rowLayout->addWidget(title);
        commands->setItemWidget(item, row);
    };

    const QVector<CommandPaletteRow> paletteRows = CommandPaletteModel::rows(
        commandRegistry,
        hasShortcutSettings ? &shortcutSettings : nullptr);
    for (const CommandPaletteRow &command : paletteRows) {
        addCommandRow(command);
    }

    layout->addWidget(input);
    layout->addWidget(commands);

    QPointer<QDialog> dialogPointer(&dialog);
    std::function<void()> selectedCommand;

    auto firstVisibleRow = [&]() {
        for (int row = 0; row < commands->count(); ++row) {
            if (!commands->item(row)->isHidden()) {
                return row;
            }
        }
        return -1;
    };

    auto updateFilter = [&]() {
        for (int row = 0; row < commands->count(); ++row) {
            auto *item = commands->item(row);
            CommandPaletteRow paletteRow;
            paletteRow.id = item->data(Qt::UserRole).toString();
            paletteRow.title = item->data(Qt::UserRole + 1).toString();
            paletteRow.shortcutText = item->data(Qt::UserRole + 2).toString();
            paletteRow.keywords = item->data(Qt::UserRole + 3).toString();
            item->setHidden(!CommandPaletteModel::matchesQuery(paletteRow, input->text()));
        }
        commands->setCurrentRow(firstVisibleRow());
    };

    auto executeCurrent = [&]() {
        auto *item = commands->currentItem();
        if (!item || item->isHidden()) {
            const int row = firstVisibleRow();
            if (row < 0) {
                return;
            }
            item = commands->item(row);
            commands->setCurrentItem(item);
        }

        const QString commandId = item->data(Qt::UserRole).toString();
        selectedCommand = [this, commandId]() { commandRegistry.triggerCommand(commandId); };
        dialog.accept();
    };

    connect(input, &QLineEdit::textChanged, &dialog, updateFilter);
    connect(input, &QLineEdit::returnPressed, &dialog, executeCurrent);
    connect(commands, &QListWidget::itemActivated, &dialog, executeCurrent);
    connect(commands, &QListWidget::itemDoubleClicked, &dialog, executeCurrent);

    updateFilter();
    input->setFocus(Qt::OtherFocusReason);

    if (dialog.exec() == QDialog::Accepted && dialogPointer && selectedCommand) {
        selectedCommand();
    }
}

void MainWindow::registerWorkbenchCommands()
{
    auto registerCommand = [this](
        const QString &id,
        const QString &title,
        const QString &category,
        const QKeySequence &shortcut,
        const QString &keywords,
        std::function<void()> trigger,
        std::function<bool()> isEnabled = {}) {
        CommandDefinition command;
        command.id = id;
        command.title = title;
        command.category = category;
        command.defaultShortcut = shortcut;
        command.keywords = keywords;
        command.isEnabled = isEnabled ? isEnabled : []() { return true; };
        command.trigger = trigger;
        commandRegistry.registerCommand(command);
    };

    registerCommand(
        QStringLiteral("file.new"),
        QString::fromUtf8("ملف جديد"),
        QString::fromUtf8("ملف"),
        QKeySequence::New,
        QString::fromUtf8("ملف جديد new file"),
        [this]() { newFile(); });
    registerCommand(
        QStringLiteral("file.open"),
        QString::fromUtf8("فتح ملف"),
        QString::fromUtf8("ملف"),
        QKeySequence::Open,
        QString::fromUtf8("فتح ملف open file"),
        [this]() { openFile(); });
    registerCommand(
        QStringLiteral("project.open"),
        QString::fromUtf8("فتح مشروع"),
        QString::fromUtf8("ملف"),
        QKeySequence(),
        QString::fromUtf8("فتح مشروع مجلد open project folder"),
        [this]() { openFolder(); });
    registerCommand(
        QStringLiteral("file.save"),
        QString::fromUtf8("حفظ"),
        QString::fromUtf8("ملف"),
        QKeySequence::Save,
        QString::fromUtf8("حفظ save"),
        [this]() { saveFile(); });
    registerCommand(
        QStringLiteral("file.saveAs"),
        QString::fromUtf8("حفظ باسم"),
        QString::fromUtf8("ملف"),
        QKeySequence::SaveAs,
        QString::fromUtf8("حفظ باسم save as"),
        [this]() { saveFileAs(); });
    registerCommand(
        QStringLiteral("document.save"),
        QString::fromUtf8("حفظ المستند"),
        QString::fromUtf8("المستند"),
        QKeySequence::Save,
        QString::fromUtf8("حفظ المستند الحالي save document"),
        [this]() { saveFile(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("document.saveAll"),
        QString::fromUtf8("حفظ كل المستندات"),
        QString::fromUtf8("المستند"),
        QKeySequence(),
        QString::fromUtf8("حفظ كل المستندات المفتوحة save all"),
        [this]() {
            const QVector<EditorSurface *> surfaces = editorTabsController ? editorTabsController->dirtySurfaces() : QVector<EditorSurface *> {};
            for (EditorSurface *surface : surfaces) {
                editorTabs->setCurrentWidget(surface);
                saveFile();
                if (surface->isDirty()) {
                    return;
                }
            }
        },
        [this]() { return editorTabs && editorTabs->count() > 0; });
    registerCommand(
        QStringLiteral("document.revert"),
        QString::fromUtf8("إعادة تحميل المستند"),
        QString::fromUtf8("المستند"),
        QKeySequence(),
        QString::fromUtf8("إعادة تحميل المستند الحالي من القرص revert reload"),
        [this]() {
            if (!editor || editor->currentFilePath().isEmpty()) {
                return;
            }
            if (editor->isDirty() && !confirmUnsavedDocuments(UnsavedChangesOperation::OpenFile)) {
                return;
            }
            const QString path = editor->currentFilePath();
            QString error;
            if (!editor->openFile(path, &error)) {
                QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
                return;
            }
            if (editorTabsController) {
                editorTabsController->syncSessionsInto(workbenchState);
            }
        },
        [this]() { return editor && !editor->currentFilePath().isEmpty(); });
    registerCommand(
        QStringLiteral("document.closeWithPrompt"),
        QString::fromUtf8("إغلاق المستند"),
        QString::fromUtf8("المستند"),
        QKeySequence::Close,
        QString::fromUtf8("إغلاق المستند الحالي مع حماية التغييرات close document"),
        [this]() {
            if (editorTabs) {
                requestCloseEditorTab(editorTabs->currentIndex());
            }
        },
        [this]() { return editorTabs && editorTabs->count() > 0; });
    registerCommand(
        QStringLiteral("editor.findInFile"),
        QString::fromUtf8("بحث واستبدال في الملف"),
        QString::fromUtf8("تحرير"),
        QKeySequence::Find,
        QString::fromUtf8("بحث استبدال في الملف find replace"),
        [this]() { openInFileFind(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("cursor.addAbove"),
        QString::fromUtf8("إضافة مؤشر أعلى"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("Ctrl+Alt+Up")),
        QString::fromUtf8("مؤشر متعدد أعلى multi cursor above"),
        [this]() { addCursorAboveAction(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("cursor.addBelow"),
        QString::fromUtf8("إضافة مؤشر أسفل"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("Ctrl+Alt+Down")),
        QString::fromUtf8("مؤشر متعدد أسفل multi cursor below"),
        [this]() { addCursorBelowAction(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("cursor.addAtNextMatch"),
        QString::fromUtf8("إضافة مؤشر عند المطابقة التالية"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("Ctrl+D")),
        QString::fromUtf8("مؤشر متعدد المطابقة التالية next match"),
        [this]() { addCursorAtNextMatchAction(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("cursor.selectAllMatches"),
        QString::fromUtf8("تحديد كل المطابقات"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("Ctrl+Shift+L")),
        QString::fromUtf8("مؤشر متعدد كل المطابقات select all matches"),
        [this]() { selectAllCursorMatchesAction(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("cursor.collapseToSingle"),
        QString::fromUtf8("الرجوع إلى مؤشر واحد"),
        QString::fromUtf8("تحرير"),
        QKeySequence(),
        QString::fromUtf8("مؤشر واحد إلغاء المؤشرات المتعددة collapse cursors"),
        [this]() { collapseToSingleCursorAction(); },
        [this]() { return editor != nullptr && editor->totalCursorCount() > 1; });
    registerCommand(
        QStringLiteral("lsp.goToDefinition"),
        QString::fromUtf8("انتقال إلى التعريف"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("F12")),
        QString::fromUtf8("تعريف رمز انتقال language server definition"),
        [this]() {
            if (!editor) {
                return;
            }
            const QTextCursor cursor = editor->textCursor();
            requestLanguageServerDefinition(cursor.blockNumber(), cursor.position() - cursor.block().position());
        },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("lsp.findReferences"),
        QString::fromUtf8("إيجاد المراجع"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("Shift+F12")),
        QString::fromUtf8("مراجع رمز language server references"),
        [this]() { requestLanguageServerReferences(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("lsp.renameSymbol"),
        QString::fromUtf8("إعادة تسمية الرمز"),
        QString::fromUtf8("تحرير"),
        QKeySequence(QStringLiteral("F2")),
        QString::fromUtf8("إعادة تسمية refactor rename symbol language server"),
        [this]() { requestLanguageServerRename(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("snippet.insertPrint"),
        QString::fromUtf8("إدراج اطبع"),
        QString::fromUtf8("تحرير"),
        QKeySequence(),
        QString::fromUtf8("إدراج مقتطف اطبع snippet print"),
        [this]() { insertPrintSnippet(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("editor.toggleVisibleWhitespace"),
        QString::fromUtf8("إظهار المسافات"),
        QString::fromUtf8("عرض"),
        QKeySequence(),
        QString::fromUtf8("إظهار إخفاء المسافات visible whitespace tabs spaces"),
        [this]() { toggleVisibleWhitespace(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("editor.toggleTrimTrailingWhitespace"),
        QString::fromUtf8("حذف فراغات آخر السطر عند الحفظ"),
        QString::fromUtf8("عرض"),
        QKeySequence(),
        QString::fromUtf8("حذف فراغات آخر السطر عند الحفظ trim trailing whitespace"),
        [this]() { toggleTrimTrailingWhitespace(); },
        [this]() { return editor != nullptr; });
    registerCommand(
        QStringLiteral("run.currentFile"),
        QString::fromUtf8("تشغيل الملف الحالي"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(QStringLiteral("F5")),
        QString::fromUtf8("تشغيل run current file"),
        [this]() { runCurrentFile(); });
    registerCommand(
        QStringLiteral("run.rerunLast"),
        QString::fromUtf8("إعادة آخر تشغيل"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(QStringLiteral("Ctrl+F5")),
        QString::fromUtf8("إعادة آخر تشغيل rerun last run history"),
        [this]() { rerunLastRuntimeAction(); },
        [this]() { return runtimeOrchestrator.hasHistory(); });
    registerCommand(
        QStringLiteral("run.lintCurrentFile"),
        QString::fromUtf8("فحص الملف الحالي"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(),
        QString::fromUtf8("فحص lint current file"),
        [this]() { lintCurrentFile(); });
    registerCommand(
        QStringLiteral("run.formatCurrentFile"),
        QString::fromUtf8("تنسيق الملف الحالي"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(),
        QString::fromUtf8("تنسيق format current file"),
        [this]() { formatCurrentFile(); });
    registerCommand(
        QStringLiteral("run.stop"),
        QString::fromUtf8("إيقاف التشغيل"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(QStringLiteral("Shift+F5")),
        QString::fromUtf8("إيقاف التشغيل stop cancel run"),
        [this]() { cancelRuntimeProcess(); },
        [this]() { return runtimeOrchestrator.isRunning(); });
    registerCommand(
        QStringLiteral("output.copy"),
        QString::fromUtf8("نسخ الإخراج"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("نسخ الإخراج copy output"),
        [this]() { copyOutputPanel(); },
        [this]() { return outputPanel && !outputPanel->toPlainText().isEmpty(); });
    registerCommand(
        QStringLiteral("output.clear"),
        QString::fromUtf8("مسح الإخراج"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("مسح الإخراج clear output"),
        [this]() { clearOutputPanel(); },
        [this]() { return outputPanel && !outputPanel->toPlainText().isEmpty(); });
    registerCommand(
        QStringLiteral("output.saveAs"),
        QString::fromUtf8("حفظ الإخراج باسم"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("حفظ الإخراج save output transcript"),
        [this]() { saveOutputPanel(); },
        [this]() { return outputPanel && !outputPanel->toPlainText().isEmpty(); });
    registerCommand(
        QStringLiteral("output.filter.all"),
        QString::fromUtf8("عرض كل الإخراج"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("عرض كل الإخراج output all"),
        [this]() { showAllOutput(); });
    registerCommand(
        QStringLiteral("output.filter.stdout"),
        QString::fromUtf8("عرض stdout فقط"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("عرض stdout فقط output filter"),
        [this]() { showOnlyStdoutOutput(); });
    registerCommand(
        QStringLiteral("output.filter.stderr"),
        QString::fromUtf8("عرض stderr فقط"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("عرض stderr فقط output filter"),
        [this]() { showOnlyStderrOutput(); });
    registerCommand(
        QStringLiteral("output.filter.system"),
        QString::fromUtf8("عرض رسائل النظام فقط"),
        QString::fromUtf8("الإخراج"),
        QKeySequence(),
        QString::fromUtf8("عرض رسائل النظام فقط output filter"),
        [this]() { showOnlySystemOutput(); });
    registerCommand(
        QStringLiteral("terminal.openPowerShell"),
        QString::fromUtf8("فتح طرفية PowerShell"),
        QString::fromUtf8("الطرفية"),
        QKeySequence(),
        QString::fromUtf8("فتح طرفية PowerShell terminal"),
        [this]() { openPowerShellTerminal(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("workspace.trust"),
        QString::fromUtf8("الثقة بمساحة العمل"),
        QString::fromUtf8("مساحة العمل"),
        QKeySequence(),
        QString::fromUtf8("الثقة بمساحة العمل workspace trust"),
        [this]() { trustCurrentWorkspace(); },
        [this]() { return !projectRoot.isEmpty() && !workspaceSettings.trusted; });
    registerCommand(
        QStringLiteral("workspace.untrust"),
        QString::fromUtf8("إلغاء الثقة بمساحة العمل"),
        QString::fromUtf8("مساحة العمل"),
        QKeySequence(),
        QString::fromUtf8("إلغاء الثقة بمساحة العمل workspace untrust revoke trust"),
        [this]() { untrustCurrentWorkspace(); },
        [this]() { return !projectRoot.isEmpty() && workspaceSettings.trusted; });
    registerCommand(
        QStringLiteral("search.project"),
        QString::fromUtf8("بحث في المشروع"),
        QString::fromUtf8("بحث"),
        QKeySequence(Qt::Key_Return),
        QString::fromUtf8("بحث في المشروع search find"),
        [this]() {
            if (!commandBox) {
                return;
            }
            commandBox->setFocus(Qt::ShortcutFocusReason);
            commandBox->selectAll();
            if (!commandBox->text().trimmed().isEmpty()) {
                findInProject();
            }
        });
    registerCommand(
        QStringLiteral("search.replacePreview"),
        QString::fromUtf8("معاينة الاستبدال في المشروع"),
        QString::fromUtf8("بحث"),
        QKeySequence(),
        QString::fromUtf8("معاينة استبدال في المشروع project replace preview"),
        [this]() { previewProjectReplace(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("search.replaceApplyAccepted"),
        QString::fromUtf8("تطبيق الاستبدالات المقبولة"),
        QString::fromUtf8("بحث"),
        QKeySequence(),
        QString::fromUtf8("تطبيق الاستبدالات المقبولة apply accepted project replace"),
        [this]() { applyAcceptedProjectReplaceRows(); },
        [this]() { return searchResultsPanel && searchResultsPanel->count() > 0; });
    registerCommand(
        QStringLiteral("project.file.new"),
        QString::fromUtf8("ملف جديد في المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("ملف جديد في المشروع project new file"),
        [this]() { if (projectTreeController) projectTreeController->createFile(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("project.folder.new"),
        QString::fromUtf8("مجلد جديد في المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("مجلد جديد في المشروع project new folder"),
        [this]() { if (projectTreeController) projectTreeController->createFolder(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.open"),
        QString::fromUtf8("فتح عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("فتح عنصر المشروع project open item"),
        [this]() { if (projectTreeController) projectTreeController->openSelectedItem(); },
        [this]() { return projectTreeController && !projectTreeController->currentSelectionPath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.rename"),
        QString::fromUtf8("إعادة تسمية عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("إعادة تسمية rename project item"),
        [this]() { if (projectTreeController) projectTreeController->renameSelectedItem(); },
        [this]() { return projectTreeController && !projectTreeController->currentSelectionPath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.deleteWithPrompt"),
        QString::fromUtf8("حذف عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("حذف مع تأكيد delete project item"),
        [this]() { if (projectTreeController) projectTreeController->deleteSelectedItem(); },
        [this]() { return projectTreeController && !projectTreeController->currentSelectionPath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.reveal"),
        QString::fromUtf8("إظهار عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("إظهار في مستكشف الملفات reveal explorer"),
        [this]() { if (projectTreeController) projectTreeController->revealSelectedItem(); },
        [this]() { return projectTreeController && !projectTreeController->currentSelectionPath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.copyPath"),
        QString::fromUtf8("نسخ مسار عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("نسخ المسار copy path"),
        [this]() { if (projectTreeController) projectTreeController->copySelectedItemPath(); },
        [this]() { return projectTreeController && !projectTreeController->currentSelectionPath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.openContainingFolder"),
        QString::fromUtf8("فتح المجلد الحاوي"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("فتح المجلد الحاوي open containing folder"),
        [this]() { if (projectTreeController) projectTreeController->openSelectedContainingFolder(); },
        [this]() { return projectTreeController && !projectTreeController->currentSelectionPath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.refresh"),
        QString::fromUtf8("تحديث المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("تحديث المشروع refresh project"),
        [this]() { if (projectTreeController) projectTreeController->refresh(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("system.settings"),
        QString::fromUtf8("الإعدادات"),
        QString::fromUtf8("النظام"),
        QKeySequence(QStringLiteral("Ctrl+,")),
        QString::fromUtf8("الإعدادات settings preferences"),
        [this]() { openSettings(); });
    registerCommand(
        QStringLiteral("system.commandPalette"),
        QString::fromUtf8("لوحة الأوامر"),
        QString::fromUtf8("النظام"),
        QKeySequence(QStringLiteral("Ctrl+Shift+P")),
        QString::fromUtf8("لوحة الأوامر command palette"),
        [this]() { openCommandPalette(); });
}

void MainWindow::insertPrintSnippet()
{
    insertSnippetById(QStringLiteral("apy.print"));
}

void MainWindow::addCursorAboveAction()
{
    if (editor) {
        editor->addCursorAbovePrimary();
    }
}

void MainWindow::addCursorBelowAction()
{
    if (editor) {
        editor->addCursorBelowPrimary();
    }
}

void MainWindow::addCursorAtNextMatchAction()
{
    if (editor) {
        editor->addCursorAtNextMatch();
    }
}

void MainWindow::selectAllCursorMatchesAction()
{
    if (editor) {
        editor->selectAllFindMatchesAsCursors();
    }
}

void MainWindow::collapseToSingleCursorAction()
{
    if (editor) {
        editor->collapseToSinglePrimaryCursor();
    }
    multiCursorSoftCapNoticeShown = false;
}

void MainWindow::toggleVisibleWhitespace()
{
    if (!editor) {
        return;
    }

    editor->setVisibleWhitespaceEnabled(!editor->isVisibleWhitespaceEnabled());
}

void MainWindow::toggleTrimTrailingWhitespace()
{
    if (!editor) {
        return;
    }

    editor->setTrimTrailingWhitespaceOnSave(!editor->trimTrailingWhitespaceOnSave());
}

void MainWindow::copyOutputPanel()
{
    if (!outputPanel) {
        return;
    }

    QApplication::clipboard()->setText(outputPanel->toPlainText());
    setStatus(QString::fromUtf8("تم نسخ الإخراج"));
}

void MainWindow::clearOutputPanel()
{
    if (!outputPanel) {
        return;
    }

    outputTranscript.clear();
    renderOutputTranscript();
    showOutputPanel();
    setStatus(QString::fromUtf8("تم مسح الإخراج"));
}

void MainWindow::saveOutputPanel()
{
    if (!outputPanel) {
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("حفظ الإخراج باسم"),
        projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
        QString::fromUtf8("ملفات نصية (*.txt);;كل الملفات (*)"));
    if (path.isEmpty()) {
        return;
    }

    if (!saveOutputPanelToPath(path)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر حفظ الإخراج"), statusLabel->text());
    }
}

bool MainWindow::saveOutputPanelToPath(const QString &path)
{
    if (!outputPanel || path.trimmed().isEmpty()) {
        return false;
    }

    QString error;
    if (!DocumentFileIO::saveUtf8Atomically(path, outputPanel->toPlainText(), &error)) {
        setStatus(error);
        return false;
    }

    setStatus(QString::fromUtf8("تم حفظ الإخراج"));
    return true;
}

bool MainWindow::exportShortcutSettingsToPath(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        return false;
    }

    QJsonObject shortcutJson = settings.shortcutSettingsJson();
    if (shortcutJson.isEmpty()) {
        shortcutJson.insert(QStringLiteral("version"), 1);
        shortcutJson.insert(QStringLiteral("shortcuts"), QJsonObject());
    }

    QString error;
    const QString jsonText = QString::fromUtf8(QJsonDocument(shortcutJson).toJson(QJsonDocument::Indented));
    if (!DocumentFileIO::saveUtf8Atomically(path, jsonText, &error)) {
        setStatus(error);
        return false;
    }

    setStatus(QString::fromUtf8("تم تصدير الاختصارات"));
    return true;
}

bool MainWindow::importShortcutSettingsFromPath(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setProperty("shortcutSettingsError", file.errorString());
        setStatus(file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const QString error = parseError.error == QJsonParseError::NoError
            ? QString::fromUtf8("ملف الاختصارات ليس JSON صالحا")
            : parseError.errorString();
        setProperty("shortcutSettingsError", error);
        setStatus(error);
        return false;
    }

    ShortcutSettingsModel importedSettings;
    QString error;
    const QJsonObject object = document.object();
    if (!importedSettings.loadJson(object, commandRegistry, &error)) {
        setProperty("shortcutSettingsError", error);
        setStatus(error);
        return false;
    }

    settings.saveShortcutSettingsJson(object);
    applyShortcutSettings();
    setProperty("shortcutSettingsError", QString());
    setStatus(QString::fromUtf8("تم استيراد الاختصارات"));
    return true;
}

bool MainWindow::resetShortcutSettingsToDefaults()
{
    settings.saveShortcutSettingsJson(QJsonObject());
    applyShortcutSettings();
    setProperty("shortcutSettingsError", QString());
    setStatus(QString::fromUtf8("تمت إعادة الاختصارات الافتراضية"));
    return true;
}

bool MainWindow::setShortcutOverrideForCommand(const QString &commandId, const QKeySequence &shortcut)
{
    ShortcutSettingsModel shortcutSettings;
    QString error;
    const QJsonObject shortcutJson = settings.shortcutSettingsJson();
    if (!shortcutJson.isEmpty() && !shortcutSettings.loadJson(shortcutJson, commandRegistry, &error)) {
        setProperty("shortcutSettingsError", error);
        setStatus(error);
        return false;
    }

    if (!shortcutSettings.setOverride(commandRegistry, commandId, shortcut, &error)) {
        setProperty("shortcutSettingsError", error);
        setStatus(error);
        return false;
    }

    settings.saveShortcutSettingsJson(shortcutSettings.toJson());
    applyShortcutSettings();
    setProperty("shortcutSettingsError", QString());
    setStatus(QString::fromUtf8("تم تحديث الاختصار"));
    return true;
}

bool MainWindow::openOutputLinkAtCursor()
{
    if (!outputPanel) {
        return false;
    }

    const int cursorPosition = outputPanel->textCursor().position();
    const QVector<TerminalLink> links = TerminalLinkParser::linksForText(outputPanel->toPlainText());
    for (const TerminalLink &link : links) {
        const int linkEnd = link.start + link.length;
        if (cursorPosition < link.start || cursorPosition > linkEnd) {
            continue;
        }

        const QString normalizedPath = QFileInfo(link.path).absoluteFilePath();
        if (!QFileInfo(normalizedPath).isFile()) {
            setStatus(QString::fromUtf8("تعذر فتح رابط الإخراج"));
            return false;
        }

        if (!openEditorFile(normalizedPath)) {
            setStatus(QString::fromUtf8("تعذر فتح رابط الإخراج"));
            return false;
        }

        goToEditorLocation(link.line, link.column);
        setStatus(QString::fromUtf8("تم فتح رابط الإخراج"));
        return true;
    }

    return false;
}

void MainWindow::showAllOutput()
{
    setOutputFilter(OutputTranscriptFilter());
}

void MainWindow::showOnlyStdoutOutput()
{
    OutputTranscriptFilter filter;
    filter.includeStderr = false;
    filter.includeSystem = false;
    setOutputFilter(filter);
}

void MainWindow::showOnlyStderrOutput()
{
    OutputTranscriptFilter filter;
    filter.includeStdout = false;
    filter.includeSystem = false;
    setOutputFilter(filter);
}

void MainWindow::showOnlySystemOutput()
{
    OutputTranscriptFilter filter;
    filter.includeStdout = false;
    filter.includeStderr = false;
    setOutputFilter(filter);
}

void MainWindow::openPowerShellTerminal()
{
    const QString workingDirectory = projectRoot.isEmpty() ? runtimeWorkingDirectory() : projectRoot;
    const TerminalProfile profile = TerminalProfileModel::defaultPowerShellProfile(workingDirectory);
    const TerminalLaunchPlan plan = TerminalProfileModel::buildLaunchPlan(profile, workspaceSettings.trusted);

    showTerminalPanel();
    if (!plan.allowed) {
        terminalPanel->setPlainText(plan.reason);
        setStatus(plan.reason);
        return;
    }

    terminalPanel->setPlainText(QString::fromUtf8("طرفية %1 جاهزة في %2. تنفيذ الطرفية سيضاف في شريحة لاحقة.")
        .arg(profile.name, QDir::toNativeSeparators(plan.command.workingDirectory)));
    setStatus(QString::fromUtf8("تم تجهيز الطرفية"));
}

void MainWindow::trustCurrentWorkspace()
{
    if (projectRoot.isEmpty()) {
        setStatus(QString::fromUtf8("لا توجد مساحة عمل مفتوحة"));
        return;
    }
    if (workspaceSettings.trusted) {
        setStatus(QString::fromUtf8("مساحة العمل موثوقة بالفعل"));
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        QString::fromUtf8("الثقة بمساحة العمل"),
        QString::fromUtf8("الثقة تتيح تشغيل أدوات مرتبطة بالمشروع مثل الطرفية. لا تثق إلا بالمشاريع التي تعرف مصدرها."),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) {
        setStatus(QString::fromUtf8("لم تتغير الثقة بمساحة العمل"));
        return;
    }

    workspaceSettings.trusted = true;
    QString error;
    if (!WorkspaceSettingsStore(projectRoot).save(workspaceSettings, &error)) {
        setStatus(error);
        return;
    }
    if (!appendWorkspaceTrustAuditEntry(projectRoot, QStringLiteral("workspace.trust.granted"), &error)) {
        setStatus(error);
        return;
    }

    setStatus(QString::fromUtf8("تمت الثقة بمساحة العمل"));
}

void MainWindow::untrustCurrentWorkspace()
{
    if (projectRoot.isEmpty()) {
        setStatus(QString::fromUtf8("لا توجد مساحة عمل مفتوحة"));
        return;
    }

    workspaceSettings.trusted = false;
    QString error;
    if (!WorkspaceSettingsStore(projectRoot).save(workspaceSettings, &error)) {
        setStatus(error);
        return;
    }
    if (!appendWorkspaceTrustAuditEntry(projectRoot, QStringLiteral("workspace.trust.revoked"), &error)) {
        setStatus(error);
        return;
    }

    setStatus(QString::fromUtf8("ألغيت الثقة بمساحة العمل"));
}

bool MainWindow::insertSnippetById(const QString &id)
{
    if (!editor) {
        return false;
    }

    ApySnippet snippet;
    if (!ApySnippetService::snippetById(id, &snippet)) {
        return false;
    }

    QTextCursor cursor = editor->textCursor();
    const int insertionStart = qMin(cursor.position(), cursor.anchor());
    cursor.insertText(snippet.body);
    if (snippet.cursorOffset >= 0 && snippet.cursorOffset <= snippet.body.size()) {
        cursor.setPosition(insertionStart + snippet.cursorOffset);
    }
    editor->setTextCursor(cursor);
    editor->setFocus(Qt::ShortcutFocusReason);
    return true;
}

void MainWindow::openInFileFind()
{
    if (!inFileFindPanel || !inFileFindInput) {
        return;
    }

    inFileFindPanel->show();
    if (editor && editor->textCursor().hasSelection() && inFileFindInput->text().isEmpty()) {
        inFileFindInput->setText(editor->textCursor().selectedText());
    } else {
        updateInFileFindMatches();
    }
    inFileFindInput->setFocus(Qt::ShortcutFocusReason);
    inFileFindInput->selectAll();
}

void MainWindow::updateInFileFindMatches()
{
    const QString query = inFileFindInput ? inFileFindInput->text() : QString();
    const int count = editor ? editor->setFindQuery(query) : 0;
    const int current = editor ? editor->currentFindMatchIndex() : -1;
    if (inFileFindStatusLabel) {
        inFileFindStatusLabel->setText(count <= 0
            ? QStringLiteral("0")
            : QStringLiteral("%1/%2").arg(current + 1).arg(count));
    }
}

void MainWindow::selectNextInFileMatch()
{
    if (editor) {
        editor->selectNextFindMatch();
    }
    if (inFileFindStatusLabel && editor) {
        const int count = editor->findMatchCount();
        const int current = editor->currentFindMatchIndex();
        inFileFindStatusLabel->setText(count <= 0
            ? QStringLiteral("0")
            : QStringLiteral("%1/%2").arg(current + 1).arg(count));
    }
}

void MainWindow::selectPreviousInFileMatch()
{
    if (editor) {
        editor->selectPreviousFindMatch();
    }
    if (inFileFindStatusLabel && editor) {
        const int count = editor->findMatchCount();
        const int current = editor->currentFindMatchIndex();
        inFileFindStatusLabel->setText(count <= 0
            ? QStringLiteral("0")
            : QStringLiteral("%1/%2").arg(current + 1).arg(count));
    }
}

void MainWindow::replaceCurrentInFileMatch()
{
    if (editor && inFileReplaceInput) {
        editor->replaceCurrentFindMatch(inFileReplaceInput->text());
    }
    updateInFileFindMatches();
}

void MainWindow::replaceAllInFileMatches()
{
    if (editor && inFileReplaceInput) {
        editor->replaceAllFindMatches(inFileReplaceInput->text());
    }
    updateInFileFindMatches();
}

void MainWindow::findInProject()
{
    if (projectRoot.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("لا يوجد مشروع"), QString::fromUtf8("افتح مشروعا قبل البحث."));
        return;
    }

    const QString query = commandBox ? commandBox->text().trimmed() : QString();
    if (query.isEmpty()) {
        return;
    }

    const QString root = projectRoot;
    const int scanCap = searchScanCap;
    const int generation = workbenchState.nextSearchGeneration();
    const QVector<SearchResultRow> immediateRows = currentEditorSearchResults(query);
    renderSearchResults(immediateRows);
    showSearchResultsPanel();
    setStatus(immediateRows.isEmpty()
        ? QString::fromUtf8("جار البحث عن: %1").arg(query)
        : QString::fromUtf8("نتائج فورية: %1، جار البحث في المشروع").arg(immediateRows.size()));

    auto *watcher = new QFutureWatcher<SearchResults>(this);
    activeSearchWatcher = watcher;
    connect(watcher, &QFutureWatcher<SearchResults>::finished, this, [this, watcher, generation, immediateRows, scanCap]() {
        const SearchResults projectResults = watcher->result();
        watcher->deleteLater();
        if (activeSearchWatcher == watcher) {
            activeSearchWatcher = nullptr;
        }
        if (!workbenchState.isCurrentSearchGeneration(generation)) {
            return;
        }
        const QVector<SearchResultRow> mergedRows = SearchService::mergeRows(immediateRows, projectResults.rows);
        renderSearchResults(mergedRows);
        setStatus(projectResults.truncatedAtFileCap
            ? QString::fromUtf8("تم اقتطاع نتائج البحث عند %1 ملف").arg(scanCap)
            : QString::fromUtf8("نتائج البحث: %1").arg(mergedRows.size()));
    });
    watcher->setFuture(QtConcurrent::run([root, query, scanCap]() {
        SearchService service;
        return service.searchWithMetadata(root, query, 1000, scanCap);
    }));
}

void MainWindow::previewProjectReplace()
{
    if (projectRoot.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("لا يوجد مشروع"), QString::fromUtf8("افتح مشروعا قبل معاينة الاستبدال."));
        return;
    }

    const QString query = commandBox ? commandBox->text().trimmed() : QString();
    if (query.isEmpty()) {
        return;
    }

    const QString replacement = projectReplaceInput ? projectReplaceInput->text() : QString();
    ProjectReplaceService service;
    const QVector<ProjectReplacePreviewRow> immediateRows = currentEditorReplacePreviewRows(query, replacement);
    const ProjectReplacePreview projectPreview = service.previewProject(projectRoot, query, replacement, 1000);
    const QVector<ProjectReplacePreviewRow> rows = ProjectReplaceService::mergePreviewRows(immediateRows, projectPreview.rows);

    renderProjectReplacePreview(rows);
    showSearchResultsPanel();
    setStatus(projectPreview.truncatedAtFileCap
        ? QString::fromUtf8("تم اقتطاع معاينة الاستبدال عند %1 ملف").arg(ProjectReplaceService::MaxScannedFiles)
        : QString::fromUtf8("معاينة الاستبدال: %1").arg(rows.size()));
}

void MainWindow::applyAcceptedProjectReplaceRows()
{
    const QVector<ProjectReplacePreviewRow> rows = acceptedProjectReplaceRows();
    if (rows.isEmpty()) {
        setStatus(QString::fromUtf8("لا توجد استبدالات محددة"));
        return;
    }
    if (hasDirtyOpenDocumentForReplaceRows(rows)) {
        setStatus(QString::fromUtf8("احفظ الملفات المفتوحة قبل تطبيق الاستبدال"));
        return;
    }

    QString error;
    const ProjectReplaceApplyResult result = ProjectReplaceService::applyAcceptedRows(rows, &error);
    if (!result.succeeded) {
        setStatus(error.isEmpty() ? QString::fromUtf8("تعذر تطبيق الاستبدال") : error);
        return;
    }

    setStatus(QString::fromUtf8("تم تطبيق %1 استبدالا في %2 ملف").arg(result.rowsApplied).arg(result.filesChanged));
}

QVector<SearchResultRow> MainWindow::currentEditorSearchResults(const QString &query) const
{
    if (!editor || query.trimmed().isEmpty()) {
        return {};
    }

    SearchService service;
    return service.searchText(editor->currentFilePath(), editor->toPlainText(), query);
}

QVector<ProjectReplacePreviewRow> MainWindow::currentEditorReplacePreviewRows(const QString &query, const QString &replacement) const
{
    if (!editor || query.trimmed().isEmpty()) {
        return {};
    }

    ProjectReplaceService service;
    return service.previewText(editor->currentFilePath(), editor->toPlainText(), query, replacement).rows;
}

void MainWindow::renderSearchResults(const QVector<SearchResultRow> &rows)
{
    searchResultsPanel->clear();
    for (const auto &row : rows) {
        auto *item = new QListWidgetItem(searchResultsPanel);
        item->setData(Qt::UserRole, row.path);
        item->setData(Qt::UserRole + 1, row.line);
        item->setData(Qt::UserRole + 2, row.preview);
        item->setToolTip(QString::fromUtf8("%1\n%2")
            .arg(QDir::toNativeSeparators(row.path.isEmpty() ? currentEditorPath() : row.path), row.preview));
        item->setText(QString::fromUtf8("%1، السطر %2").arg(QFileInfo(row.path).fileName()).arg(row.line));

        auto *rowWidget = new QWidget(searchResultsPanel);
        rowWidget->setObjectName(QStringLiteral("searchResultRow"));
        rowWidget->setLayoutDirection(Qt::RightToLeft);
        rowWidget->setMinimumHeight(72);
        auto *rowLayout = new QVBoxLayout(rowWidget);
        rowLayout->setContentsMargins(16, 16, 16, 16);
        rowLayout->setSpacing(0);

        auto *metadataCluster = new QWidget(rowWidget);
        metadataCluster->setObjectName(QStringLiteral("searchResultMetadataCluster"));
        metadataCluster->setLayoutDirection(Qt::RightToLeft);
        metadataCluster->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *metaLayout = new QHBoxLayout(metadataCluster);
        metaLayout->setDirection(QBoxLayout::RightToLeft);
        metaLayout->setContentsMargins(0, 0, 0, 0);
        metaLayout->setSpacing(8);
        const QString fileLabelText = row.path.isEmpty()
            ? QString::fromUtf8("المحرر الحالي")
            : QDir::toNativeSeparators(row.path);
        auto *fileLabel = new QLabel(fileLabelText, rowWidget);
        fileLabel->setObjectName(QStringLiteral("searchResultFileLabel"));
        fileLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        fileLabel->setLayoutDirection(row.path.isEmpty() ? Qt::RightToLeft : Qt::LeftToRight);
        fileLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        fileLabel->setStyleSheet(QStringLiteral("color: #E8ECF2; font-weight: 600;"));
        auto *lineLabel = new QLabel(QString::fromUtf8("السطر %1").arg(row.line), rowWidget);
        lineLabel->setObjectName(QStringLiteral("searchResultLineLabel"));
        lineLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lineLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        lineLabel->setStyleSheet(QStringLiteral("color: #AEC6FF;"));
        metaLayout->addWidget(fileLabel, 1);
        metaLayout->addWidget(lineLabel);

        auto *detailLabel = new QLabel(QString::fromUtf8("مطابقة واحدة - انقر للفتح"), rowWidget);
        detailLabel->setObjectName(QStringLiteral("searchResultDetailLabel"));
        detailLabel->setLayoutDirection(Qt::RightToLeft);
        detailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        detailLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        detailLabel->setStyleSheet(QStringLiteral("color: #9AA7B6;"));
        auto *detailLayout = new QHBoxLayout;
        detailLayout->setDirection(QBoxLayout::RightToLeft);
        detailLayout->setContentsMargins(0, 0, 0, 0);
        detailLayout->setSpacing(0);
        detailLayout->addStretch(1);
        detailLayout->addWidget(detailLabel);

        rowLayout->addWidget(metadataCluster);
        rowLayout->addLayout(detailLayout);
        item->setSizeHint(QSize(rowWidget->sizeHint().width(), 72));
        searchResultsPanel->setItemWidget(item, rowWidget);
    }
    showSearchResultsPanel();
}

void MainWindow::renderReferences(const QVector<LspLocation> &locations)
{
    referencesPanel->clear();
    for (const LspLocation &location : locations) {
        const QString path = QUrl(location.uri).toLocalFile();
        auto *item = new QListWidgetItem(referencesPanel);
        item->setData(Qt::UserRole, location.uri);
        item->setData(Qt::UserRole + 1, location.line + 1);
        item->setData(Qt::UserRole + 2, location.character + 1);
        item->setToolTip(QString::fromUtf8("%1\n%2:%3")
            .arg(QDir::toNativeSeparators(path.isEmpty() ? location.uri : path))
            .arg(location.line + 1)
            .arg(location.character + 1));
        item->setText(QString::fromUtf8("%1، السطر %2").arg(path.isEmpty() ? location.uri : QFileInfo(path).fileName()).arg(location.line + 1));

        auto *rowWidget = new QWidget(referencesPanel);
        rowWidget->setObjectName(QStringLiteral("referenceResultRow"));
        rowWidget->setLayoutDirection(Qt::RightToLeft);
        rowWidget->setMinimumHeight(64);
        auto *rowLayout = new QVBoxLayout(rowWidget);
        rowLayout->setContentsMargins(16, 12, 16, 12);
        rowLayout->setSpacing(4);

        auto *fileLabel = new QLabel(path.isEmpty() ? location.uri : QDir::toNativeSeparators(path), rowWidget);
        fileLabel->setObjectName(QStringLiteral("referenceResultFileLabel"));
        fileLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        fileLabel->setLayoutDirection(path.isEmpty() ? Qt::LeftToRight : Qt::LeftToRight);
        fileLabel->setStyleSheet(QStringLiteral("color: #E8ECF2; font-weight: 600;"));

        auto *lineLabel = new QLabel(QString::fromUtf8("السطر %1، العمود %2").arg(location.line + 1).arg(location.character + 1), rowWidget);
        lineLabel->setObjectName(QStringLiteral("referenceResultLineLabel"));
        lineLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lineLabel->setLayoutDirection(Qt::RightToLeft);
        lineLabel->setStyleSheet(QStringLiteral("color: #AEC6FF;"));

        rowLayout->addWidget(fileLabel);
        rowLayout->addWidget(lineLabel);
        item->setSizeHint(QSize(rowWidget->sizeHint().width(), 64));
        referencesPanel->setItemWidget(item, rowWidget);
    }
    showReferencesPanel();
}

void MainWindow::renderRenamePreview(const LspWorkspaceEdit &edit)
{
    referencesPanel->clear();
    for (const LspTextEdit &textEdit : edit.edits) {
        const QString path = QUrl(textEdit.uri).toLocalFile();
        auto *item = new QListWidgetItem(referencesPanel);
        item->setData(Qt::UserRole, textEdit.uri);
        item->setData(Qt::UserRole + 1, textEdit.startLine + 1);
        item->setData(Qt::UserRole + 2, textEdit.startCharacter + 1);
        item->setToolTip(QString::fromUtf8("%1\n%2:%3 -> %4")
            .arg(QDir::toNativeSeparators(path.isEmpty() ? textEdit.uri : path))
            .arg(textEdit.startLine + 1)
            .arg(textEdit.startCharacter + 1)
            .arg(textEdit.newText));
        item->setText(QString::fromUtf8("إعادة تسمية: %1، السطر %2")
            .arg(path.isEmpty() ? textEdit.uri : QFileInfo(path).fileName())
            .arg(textEdit.startLine + 1));

        auto *rowWidget = new QWidget(referencesPanel);
        rowWidget->setObjectName(QStringLiteral("renamePreviewRow"));
        rowWidget->setLayoutDirection(Qt::RightToLeft);
        rowWidget->setMinimumHeight(72);
        auto *rowLayout = new QVBoxLayout(rowWidget);
        rowLayout->setContentsMargins(16, 12, 16, 12);
        rowLayout->setSpacing(4);

        auto *fileLabel = new QLabel(path.isEmpty() ? textEdit.uri : QDir::toNativeSeparators(path), rowWidget);
        fileLabel->setObjectName(QStringLiteral("renamePreviewFileLabel"));
        fileLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        fileLabel->setLayoutDirection(Qt::LeftToRight);
        fileLabel->setStyleSheet(QStringLiteral("color: #E8ECF2; font-weight: 600;"));

        auto *detailLabel = new QLabel(QString::fromUtf8("السطر %1، العمود %2 -> %3")
                .arg(textEdit.startLine + 1)
                .arg(textEdit.startCharacter + 1)
                .arg(textEdit.newText),
            rowWidget);
        detailLabel->setObjectName(QStringLiteral("renamePreviewDetailLabel"));
        detailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        detailLabel->setLayoutDirection(Qt::RightToLeft);
        detailLabel->setStyleSheet(QStringLiteral("color: #AEC6FF;"));

        rowLayout->addWidget(fileLabel);
        rowLayout->addWidget(detailLabel);
        item->setSizeHint(QSize(rowWidget->sizeHint().width(), 72));
        referencesPanel->setItemWidget(item, rowWidget);
    }
    showReferencesPanel();
}

bool MainWindow::applyWorkspaceEdit(const LspWorkspaceEdit &edit, QString *error)
{
    if (error) {
        error->clear();
    }
    if (edit.edits.isEmpty()) {
        setStatus(QString::fromUtf8("لا توجد تعديلات لإعادة التسمية"));
        return true;
    }

    QMap<QString, QVector<LspTextEdit>> editsByPath;
    for (const LspTextEdit &textEdit : edit.edits) {
        const QString localPath = QUrl(textEdit.uri).toLocalFile();
        if (localPath.isEmpty()) {
            if (error) {
                *error = QString::fromUtf8("تعديل إعادة التسمية لا يحتوي مسارا صالحا.");
            }
            return false;
        }
        const QString path = QFileInfo(localPath).absoluteFilePath();
        editsByPath[path].append(textEdit);
    }

    const QStringList dirtyPaths = editorTabsController ? editorTabsController->dirtyFilePaths() : QStringList {};
    for (const QString &dirtyPath : dirtyPaths) {
        const QString normalizedDirtyPath = QFileInfo(dirtyPath).absoluteFilePath();
        if (editsByPath.contains(normalizedDirtyPath)) {
            if (error) {
                *error = QString::fromUtf8("احفظ الملفات المفتوحة قبل إعادة التسمية.");
            }
            setStatus(error ? *error : QString());
            return false;
        }
    }

    struct PreparedFile
    {
        QString originalText;
        QString updatedText;
    };

    QMap<QString, PreparedFile> preparedFiles;
    for (auto it = editsByPath.begin(); it != editsByPath.end(); ++it) {
        const QString path = it.key();
        if (!QFileInfo::exists(path)) {
            if (error) {
                *error = QString::fromUtf8("ملف إعادة التسمية غير موجود: %1").arg(QDir::toNativeSeparators(path));
            }
            return false;
        }

        QString loadError;
        const DocumentLoadResult loaded = DocumentFileIO::loadUtf8(path, &loadError);
        if (!loadError.isEmpty()) {
            if (error) {
                *error = loadError;
            }
            return false;
        }

        QVector<DocumentTextEdit> documentEdits;
        documentEdits.reserve(it.value().size());
        for (const LspTextEdit &lspEdit : it.value()) {
            DocumentTextEdit documentEdit;
            if (!textEditToDocumentEdit(loaded.text, lspEdit, &documentEdit)) {
                if (error) {
                    *error = QString::fromUtf8("نطاق إعادة التسمية غير صالح: %1").arg(QDir::toNativeSeparators(path));
                }
                return false;
            }
            documentEdits.append(documentEdit);
        }

        std::sort(documentEdits.begin(), documentEdits.end(), [](const DocumentTextEdit &left, const DocumentTextEdit &right) {
            return left.start < right.start;
        });
        int previousEnd = -1;
        for (const DocumentTextEdit &documentEdit : documentEdits) {
            if (documentEdit.start < previousEnd) {
                if (error) {
                    *error = QString::fromUtf8("تعديلات إعادة التسمية متداخلة: %1").arg(QDir::toNativeSeparators(path));
                }
                return false;
            }
            previousEnd = documentEdit.start + documentEdit.length;
        }

        QString updatedText = loaded.text;
        std::sort(documentEdits.begin(), documentEdits.end(), [](const DocumentTextEdit &left, const DocumentTextEdit &right) {
            return left.start > right.start;
        });
        for (const DocumentTextEdit &documentEdit : documentEdits) {
            updatedText.replace(documentEdit.start, documentEdit.length, documentEdit.replacement);
        }
        preparedFiles.insert(path, {loaded.text, updatedText});
    }

    QStringList writtenPaths;
    for (auto it = preparedFiles.begin(); it != preparedFiles.end(); ++it) {
        QString saveError;
        if (!DocumentFileIO::saveUtf8Atomically(it.key(), it.value().updatedText, &saveError)) {
            for (const QString &writtenPath : writtenPaths) {
                QString ignoredError;
                DocumentFileIO::saveUtf8Atomically(writtenPath, preparedFiles.value(writtenPath).originalText, &ignoredError);
            }
            if (error) {
                *error = saveError;
            }
            setStatus(saveError);
            return false;
        }
        writtenPaths.append(it.key());
    }

    for (const QString &path : writtenPaths) {
        const DocumentId id = documentRegistry.findByPath(path);
        if (!id.isValid()) {
            continue;
        }
        QString reloadError;
        documentRegistry.reloadFromDisk(id, &reloadError);
        EditorSurface *surface = editorTabsController ? editorTabsController->surfaceForDocument(id) : nullptr;
        if (surface) {
            surface->openFile(path, &reloadError);
        }
    }

    renderRenamePreview(edit);
    setStatus(QString::fromUtf8("تم تطبيق %1 تعديلات إعادة تسمية في %2 ملف").arg(edit.edits.size()).arg(preparedFiles.size()));
    return true;
}

void MainWindow::renderProjectReplacePreview(const QVector<ProjectReplacePreviewRow> &rows)
{
    searchResultsPanel->clear();
    currentProjectReplacePreviewRows = rows;
    ProjectReplaceSelectionState selection = ProjectReplaceService::selectionFromRows(rows);
    for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        const ProjectReplacePreviewRow &row = rows.at(rowIndex);
        auto *item = new QListWidgetItem(searchResultsPanel);
        item->setData(Qt::UserRole, row.path);
        item->setData(Qt::UserRole + 1, row.line);
        item->setData(Qt::UserRole + 2, row.before);
        item->setData(Qt::UserRole + 3, row.after);
        item->setData(Qt::UserRole + 4, selection.isRowAccepted(rowIndex));
        item->setToolTip(QString::fromUtf8("%1\n%2\n→ %3")
            .arg(QDir::toNativeSeparators(row.path.isEmpty() ? currentEditorPath() : row.path), row.before, row.after));
        item->setText(QString::fromUtf8("%1، السطر %2").arg(QFileInfo(row.path).fileName()).arg(row.line));

        auto *rowWidget = new QWidget(searchResultsPanel);
        rowWidget->setObjectName(QStringLiteral("searchResultRow"));
        rowWidget->setLayoutDirection(Qt::RightToLeft);
        rowWidget->setMinimumHeight(112);
        auto *rowLayout = new QVBoxLayout(rowWidget);
        rowLayout->setContentsMargins(16, 14, 16, 14);
        rowLayout->setSpacing(4);

        auto *metadataCluster = new QWidget(rowWidget);
        metadataCluster->setObjectName(QStringLiteral("searchResultMetadataCluster"));
        metadataCluster->setLayoutDirection(Qt::RightToLeft);
        auto *metaLayout = new QHBoxLayout(metadataCluster);
        metaLayout->setDirection(QBoxLayout::RightToLeft);
        metaLayout->setContentsMargins(0, 0, 0, 0);
        metaLayout->setSpacing(8);

        const QString fileLabelText = row.path.isEmpty()
            ? QString::fromUtf8("المحرر الحالي")
            : QDir::toNativeSeparators(row.path);
        auto *fileLabel = new QLabel(fileLabelText, rowWidget);
        fileLabel->setObjectName(QStringLiteral("searchResultFileLabel"));
        fileLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        fileLabel->setLayoutDirection(row.path.isEmpty() ? Qt::RightToLeft : Qt::LeftToRight);
        fileLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        fileLabel->setStyleSheet(QStringLiteral("color: #E8ECF2; font-weight: 600;"));

        auto *lineLabel = new QLabel(QString::fromUtf8("السطر %1").arg(row.line), rowWidget);
        lineLabel->setObjectName(QStringLiteral("searchResultLineLabel"));
        lineLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lineLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        lineLabel->setStyleSheet(QStringLiteral("color: #AEC6FF;"));
        metaLayout->addWidget(fileLabel, 1);
        metaLayout->addWidget(lineLabel);

        auto *detailLabel = new QLabel(QString::fromUtf8("معاينة استبدال فقط - لن يتم تعديل الملف"), rowWidget);
        detailLabel->setObjectName(QStringLiteral("searchResultDetailLabel"));
        detailLabel->setLayoutDirection(Qt::RightToLeft);
        detailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        detailLabel->setStyleSheet(QStringLiteral("color: #9AA7B6;"));

        auto *beforeLabel = new QLabel(row.before, rowWidget);
        beforeLabel->setObjectName(QStringLiteral("projectReplaceBeforeLabel"));
        beforeLabel->setLayoutDirection(Qt::RightToLeft);
        beforeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        beforeLabel->setStyleSheet(QStringLiteral("color: #C8D0DC; font-family: 'Cascadia Code', 'Consolas';"));

        auto *afterLabel = new QLabel(row.after, rowWidget);
        afterLabel->setObjectName(QStringLiteral("projectReplaceAfterLabel"));
        afterLabel->setLayoutDirection(Qt::RightToLeft);
        afterLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        afterLabel->setStyleSheet(QStringLiteral("color: #B8F5C8; font-family: 'Cascadia Code', 'Consolas';"));

        auto *acceptCheckBox = new QCheckBox(QString::fromUtf8("تضمين"), rowWidget);
        acceptCheckBox->setObjectName(QStringLiteral("projectReplaceAcceptCheckBox"));
        acceptCheckBox->setLayoutDirection(Qt::RightToLeft);
        acceptCheckBox->setChecked(selection.isRowAccepted(rowIndex));
        connect(acceptCheckBox, &QCheckBox::toggled, this, [item](bool checked) {
            item->setData(Qt::UserRole + 4, checked);
        });

        auto *fileControls = new QWidget(rowWidget);
        fileControls->setObjectName(QStringLiteral("projectReplaceFileControls"));
        fileControls->setLayoutDirection(Qt::RightToLeft);
        auto *fileControlsLayout = new QHBoxLayout(fileControls);
        fileControlsLayout->setContentsMargins(0, 0, 0, 0);
        fileControlsLayout->setSpacing(6);
        auto *acceptFileButton = new QPushButton(QString::fromUtf8("تضمين الملف"), fileControls);
        acceptFileButton->setObjectName(QStringLiteral("projectReplaceAcceptFileButton"));
        auto *rejectFileButton = new QPushButton(QString::fromUtf8("استبعاد الملف"), fileControls);
        rejectFileButton->setObjectName(QStringLiteral("projectReplaceRejectFileButton"));
        connect(acceptFileButton, &QPushButton::clicked, this, [this, path = row.path]() {
            setProjectReplaceFileAccepted(path, true);
        });
        connect(rejectFileButton, &QPushButton::clicked, this, [this, path = row.path]() {
            setProjectReplaceFileAccepted(path, false);
        });
        fileControlsLayout->addWidget(acceptFileButton);
        fileControlsLayout->addWidget(rejectFileButton);
        fileControlsLayout->addStretch(1);

        rowLayout->addWidget(metadataCluster);
        rowLayout->addWidget(acceptCheckBox);
        rowLayout->addWidget(fileControls);
        rowLayout->addWidget(detailLabel);
        rowLayout->addWidget(beforeLabel);
        rowLayout->addWidget(afterLabel);
        item->setSizeHint(QSize(rowWidget->sizeHint().width(), 160));
        searchResultsPanel->setItemWidget(item, rowWidget);
    }
    showSearchResultsPanel();
}

void MainWindow::setProjectReplaceFileAccepted(const QString &path, bool accepted)
{
    if (!searchResultsPanel) {
        return;
    }

    for (int row = 0; row < searchResultsPanel->count(); ++row) {
        auto *item = searchResultsPanel->item(row);
        if (!item || item->data(Qt::UserRole).toString() != path) {
            continue;
        }

        item->setData(Qt::UserRole + 4, accepted);
        auto *rowWidget = searchResultsPanel->itemWidget(item);
        auto *checkBox = rowWidget ? rowWidget->findChild<QCheckBox *>(QStringLiteral("projectReplaceAcceptCheckBox")) : nullptr;
        if (checkBox && checkBox->isChecked() != accepted) {
            checkBox->setChecked(accepted);
        }
    }
}

QVector<ProjectReplacePreviewRow> MainWindow::acceptedProjectReplaceRows() const
{
    QVector<ProjectReplacePreviewRow> rows;
    if (!searchResultsPanel) {
        return rows;
    }

    for (int row = 0; row < searchResultsPanel->count(); ++row) {
        auto *item = searchResultsPanel->item(row);
        if (!item || !item->data(Qt::UserRole + 4).toBool()) {
            continue;
        }
        rows.push_back({
            item->data(Qt::UserRole).toString(),
            item->data(Qt::UserRole + 1).toInt(),
            item->data(Qt::UserRole + 2).toString(),
            item->data(Qt::UserRole + 3).toString(),
            1,
        });
    }
    return rows;
}

bool MainWindow::hasDirtyOpenDocumentForReplaceRows(const QVector<ProjectReplacePreviewRow> &rows) const
{
    const QStringList dirtyPaths = editorTabsController ? editorTabsController->dirtyFilePaths() : QStringList {};
    for (const QString &openPath : dirtyPaths) {
        for (const ProjectReplacePreviewRow &row : rows) {
            if (!row.path.isEmpty() && QFileInfo(row.path).absoluteFilePath() == openPath) {
                return true;
            }
        }
    }
    return false;
}

void MainWindow::openSearchResult(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    const QString path = item->data(Qt::UserRole).toString();
    const int line = item->data(Qt::UserRole + 1).toInt();
    if (!path.isEmpty() && QFileInfo(path).isFile()) {
        openEditorFile(path);
    }
    goToEditorLine(line);
}

void MainWindow::openProblemResult(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    const QString path = item->data(Qt::UserRole).toString();
    const int line = item->data(Qt::UserRole + 1).toInt();
    if (!path.isEmpty() && QFileInfo(path).isFile()) {
        openEditorFile(path);
    }
    goToEditorLine(line);
}

void MainWindow::openReferenceResult(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    const QString uri = item->data(Qt::UserRole).toString();
    const QString path = QUrl(uri).toLocalFile();
    const int line = item->data(Qt::UserRole + 1).toInt();
    const int column = item->data(Qt::UserRole + 2).toInt();
    if (!path.isEmpty() && QFileInfo(path).isFile()) {
        openEditorFile(path);
    }
    goToEditorLocation(line, column);
}

void MainWindow::clearEditorsForDeletedPath(const QString &path)
{
    if (editorTabsController) {
        editorTabsController->closeTabsMatching([&path](EditorSurface *surface) {
            const QString openPath = QFileInfo(surface->currentFilePath()).absoluteFilePath();
            return !openPath.isEmpty() && ProjectModel::pathIsSameOrInside(openPath, path);
        });
    }
    refreshEditorProblems();
}

void MainWindow::openSettings()
{
    const QStringList orderedFamilies = arabicEditorFontFamilies();
    RuntimeDiagnostics diagnostics;
    diagnostics.pythonExecutable = runtimeOrchestrator.pythonExecutablePath();
    diagnostics.statusText = QString::fromUtf8("جار فحص التشغيل...");
    const SettingsDialogState settingsState = SettingsDialogModel::build(settings, diagnostics, orderedFamilies);

    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("settingsDialog"));
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setWindowTitle(QString::fromUtf8("الإعدادات"));
    dialog.setLayoutDirection(Qt::RightToLeft);
    dialog.resize(760, 500);

    auto *rootLayout = new QVBoxLayout(&dialog);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setDirection(QBoxLayout::RightToLeft);
    auto *headerTitle = new QLabel(QString::fromUtf8("الإعدادات"), &dialog);
    headerTitle->setObjectName(QStringLiteral("settingsHeaderTitle"));
    headerTitle->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 700; color: #E8ECF2;"));
    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch(1);
    rootLayout->addLayout(headerLayout);

    auto *contentLayout = new QHBoxLayout;
    contentLayout->setDirection(QBoxLayout::RightToLeft);
    contentLayout->setSpacing(12);

    auto *categories = new QListWidget(&dialog);
    categories->setObjectName(QStringLiteral("settingsCategories"));
    categories->setLayoutDirection(Qt::RightToLeft);
    categories->setFixedWidth(180);
    categories->addItems(settingsState.categories);

    auto *pages = new QTabWidget(&dialog);
    pages->setObjectName(QStringLiteral("settingsPages"));
    pages->setLayoutDirection(Qt::RightToLeft);
    pages->tabBar()->hide();

    auto *editorPage = new QWidget(pages);
    editorPage->setObjectName(QStringLiteral("editorSettingsPage"));
    editorPage->setLayoutDirection(Qt::RightToLeft);
    auto *editorForm = new QFormLayout(editorPage);
    editorForm->setLabelAlignment(Qt::AlignRight);
    auto *fontFamilyCombo = new QComboBox(editorPage);
    fontFamilyCombo->setObjectName(QStringLiteral("editorFontFamilyCombo"));
    fontFamilyCombo->setLayoutDirection(Qt::RightToLeft);
    fontFamilyCombo->setEditable(false);
    fontFamilyCombo->addItems(settingsState.editorFontFamilies);
    const int configuredFontIndex = fontFamilyCombo->findText(settingsState.selectedEditorFontFamily);
    fontFamilyCombo->setCurrentIndex(configuredFontIndex >= 0 ? configuredFontIndex : 0);
    auto *fontSizeInput = new QSpinBox(editorPage);
    fontSizeInput->setObjectName(QStringLiteral("editorFontSizeInput"));
    fontSizeInput->setRange(8, 28);
    fontSizeInput->setValue(settingsState.editorFontSize);
    auto *themePreferenceCombo = new QComboBox(editorPage);
    themePreferenceCombo->setObjectName(QStringLiteral("themePreferenceCombo"));
    themePreferenceCombo->setLayoutDirection(Qt::RightToLeft);
    themePreferenceCombo->addItem(QString::fromUtf8("داكن"), QStringLiteral("dark"));
    themePreferenceCombo->addItem(QString::fromUtf8("فاتح"), QStringLiteral("light"));
    const int configuredThemeIndex = themePreferenceCombo->findData(settings.themePreference());
    themePreferenceCombo->setCurrentIndex(configuredThemeIndex >= 0 ? configuredThemeIndex : 0);
    editorForm->addRow(QString::fromUtf8("خط المحرر"), fontFamilyCombo);
    editorForm->addRow(QString::fromUtf8("حجم الخط"), fontSizeInput);
    editorForm->addRow(QString::fromUtf8("السمة"), themePreferenceCombo);

    auto *runtimePage = new QWidget(pages);
    runtimePage->setObjectName(QStringLiteral("runtimeDiagnosticsPage"));
    runtimePage->setLayoutDirection(Qt::RightToLeft);
    auto *runtimeForm = new QFormLayout(runtimePage);
    runtimeForm->setLabelAlignment(Qt::AlignRight);
    auto *runtimePythonPath = new QLabel(settingsState.runtimePythonPath, runtimePage);
    runtimePythonPath->setObjectName(QStringLiteral("runtimePythonPathValue"));
    runtimePythonPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *runtimePackageStatus = new QLabel(settingsState.runtimePackageStatus, runtimePage);
    runtimePackageStatus->setObjectName(QStringLiteral("runtimePackageStatusValue"));
    runtimePackageStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *runtimeRunStatus = new QLabel(settingsState.runtimeRunStatus, runtimePage);
    runtimeRunStatus->setObjectName(QStringLiteral("runtimeRunStatusValue"));
    auto *runtimeLintStatus = new QLabel(settingsState.runtimeLintStatus, runtimePage);
    runtimeLintStatus->setObjectName(QStringLiteral("runtimeLintStatusValue"));
    auto *runtimeFormatStatus = new QLabel(settingsState.runtimeFormatStatus, runtimePage);
    runtimeFormatStatus->setObjectName(QStringLiteral("runtimeFormatStatusValue"));
    runtimeForm->addRow(QString::fromUtf8("مسار Python"), runtimePythonPath);
    runtimeForm->addRow(QString::fromUtf8("حزمة لغة الثعبان"), runtimePackageStatus);
    runtimeForm->addRow(QString::fromUtf8("تشغيل .apy"), runtimeRunStatus);
    runtimeForm->addRow(QString::fromUtf8("الفحص"), runtimeLintStatus);
    runtimeForm->addRow(QString::fromUtf8("التنسيق"), runtimeFormatStatus);

    auto applyRuntimeDiagnostics = [this,
                                    orderedFamilies,
                                    runtimePythonPath,
                                    runtimePackageStatus,
                                    runtimeRunStatus,
                                    runtimeLintStatus,
                                    runtimeFormatStatus](const RuntimeDiagnostics &latestDiagnostics) {
        const SettingsDialogState latestState = SettingsDialogModel::build(settings, latestDiagnostics, orderedFamilies);
        runtimePythonPath->setText(latestState.runtimePythonPath);
        runtimePackageStatus->setText(latestState.runtimePackageStatus);
        runtimeRunStatus->setText(latestState.runtimeRunStatus);
        runtimeLintStatus->setText(latestState.runtimeLintStatus);
        runtimeFormatStatus->setText(latestState.runtimeFormatStatus);
    };

    auto *diagnosticsWatcher = new QFutureWatcher<RuntimeDiagnostics>(&dialog);
    connect(diagnosticsWatcher, &QFutureWatcher<RuntimeDiagnostics>::finished, &dialog, [diagnosticsWatcher, applyRuntimeDiagnostics]() {
        applyRuntimeDiagnostics(diagnosticsWatcher->result());
        diagnosticsWatcher->deleteLater();
    });
    const QString runtimeRoot = runtimeOrchestrator.runtimeRoot();
    const auto diagnosticsProvider = runtimeDiagnosticsProvider;
    diagnosticsWatcher->setFuture(QtConcurrent::run([diagnosticsProvider, runtimeRoot]() {
        if (diagnosticsProvider) {
            return diagnosticsProvider(1500);
        }

        RuntimeRunner probe;
        probe.setRuntimeRoot(runtimeRoot);
        return probe.diagnostics(1500);
    }));

    auto *projectsPage = new QWidget(pages);
    projectsPage->setObjectName(QStringLiteral("recentProjectsPage"));
    projectsPage->setLayoutDirection(Qt::RightToLeft);
    auto *projectsLayout = new QVBoxLayout(projectsPage);
    auto *recentProjects = new QListWidget(projectsPage);
    recentProjects->setObjectName(QStringLiteral("recentProjectsList"));
    recentProjects->setLayoutDirection(Qt::RightToLeft);
    recentProjects->addItems(settingsState.recentProjects);
    projectsLayout->addWidget(recentProjects);

    auto *shortcutsPage = new QWidget(pages);
    shortcutsPage->setObjectName(QStringLiteral("shortcutSettingsPage"));
    shortcutsPage->setLayoutDirection(Qt::RightToLeft);
    auto *shortcutsLayout = new QVBoxLayout(shortcutsPage);
    shortcutsLayout->setContentsMargins(8, 8, 8, 8);
    shortcutsLayout->setSpacing(8);
    auto *shortcutList = new QListWidget(shortcutsPage);
    shortcutList->setObjectName(QStringLiteral("shortcutSettingsList"));
    shortcutList->setLayoutDirection(Qt::RightToLeft);
    shortcutList->setUniformItemSizes(false);

    ShortcutSettingsModel shortcutSettings;
    const QJsonObject shortcutJson = settings.shortcutSettingsJson();
    const bool hasShortcutSettings = !shortcutJson.isEmpty() && shortcutSettings.loadJson(shortcutJson, commandRegistry);
    for (const CommandDefinition &command : commandRegistry.commands()) {
        const QKeySequence shortcutSequence = hasShortcutSettings
            ? shortcutSettings.effectiveShortcut(commandRegistry, command.id)
            : command.defaultShortcut;
        const QString shortcutText = shortcutSequence.toString(QKeySequence::NativeText);
        auto *item = new QListWidgetItem(
            QStringLiteral("%1    %2").arg(command.title, shortcutText),
            shortcutList);
        item->setData(Qt::UserRole, command.id);
        item->setData(Qt::UserRole + 1, shortcutText);
        item->setData(Qt::UserRole + 2, command.title);
        item->setToolTip(command.id);
    }
    shortcutsLayout->addWidget(shortcutList, 1);

    auto *shortcutEditRow = new QWidget(shortcutsPage);
    shortcutEditRow->setLayoutDirection(Qt::RightToLeft);
    auto *shortcutEditLayout = new QHBoxLayout(shortcutEditRow);
    shortcutEditLayout->setContentsMargins(0, 0, 0, 0);
    auto *shortcutEditInput = new QKeySequenceEdit(shortcutEditRow);
    shortcutEditInput->setObjectName(QStringLiteral("shortcutEditInput"));
    shortcutEditInput->setLayoutDirection(Qt::LeftToRight);
    auto *shortcutApplyButton = new QPushButton(QString::fromUtf8("تعيين"), shortcutEditRow);
    shortcutApplyButton->setObjectName(QStringLiteral("shortcutApplyButton"));
    shortcutApplyButton->setEnabled(false);
    shortcutEditLayout->addWidget(shortcutEditInput, 1);
    shortcutEditLayout->addWidget(shortcutApplyButton);
    shortcutsLayout->addWidget(shortcutEditRow);

    connect(shortcutList, &QListWidget::currentItemChanged, this, [shortcutEditInput, shortcutApplyButton](QListWidgetItem *current) {
        shortcutApplyButton->setEnabled(current != nullptr);
        shortcutEditInput->setKeySequence(current ? QKeySequence(current->data(Qt::UserRole + 1).toString()) : QKeySequence());
    });
    connect(shortcutApplyButton, &QPushButton::clicked, this, [this, shortcutList, shortcutEditInput]() {
        auto *item = shortcutList->currentItem();
        if (!item) {
            return;
        }
        const QString commandId = item->data(Qt::UserRole).toString();
        const QKeySequence shortcut = shortcutEditInput->keySequence();
        if (!setShortcutOverrideForCommand(commandId, shortcut)) {
            return;
        }
        const QString shortcutText = shortcut.toString(QKeySequence::NativeText);
        item->setData(Qt::UserRole + 1, shortcutText);
        item->setText(QStringLiteral("%1    %2").arg(item->data(Qt::UserRole + 2).toString(), shortcutText));
    });

    auto *shortcutButtonRow = new QWidget(shortcutsPage);
    shortcutButtonRow->setLayoutDirection(Qt::RightToLeft);
    auto *shortcutButtonLayout = new QHBoxLayout(shortcutButtonRow);
    shortcutButtonLayout->setContentsMargins(0, 0, 0, 0);
    shortcutButtonLayout->addStretch(1);
    auto *shortcutImportButton = new QPushButton(QString::fromUtf8("استيراد"), shortcutButtonRow);
    shortcutImportButton->setObjectName(QStringLiteral("shortcutImportButton"));
    connect(shortcutImportButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this,
            QString::fromUtf8("استيراد الاختصارات"),
            projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
            QString::fromUtf8("ملفات اختصارات لسان (*.json);;كل الملفات (*)"));
        if (path.isEmpty()) {
            return;
        }
        if (!importShortcutSettingsFromPath(path)) {
            QMessageBox::warning(this, QString::fromUtf8("تعذر استيراد الاختصارات"), statusLabel->text());
        }
    });
    auto *shortcutExportButton = new QPushButton(QString::fromUtf8("تصدير"), shortcutButtonRow);
    shortcutExportButton->setObjectName(QStringLiteral("shortcutExportButton"));
    connect(shortcutExportButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(
            this,
            QString::fromUtf8("تصدير الاختصارات"),
            projectRoot.isEmpty() ? QDir::homePath() : projectRoot,
            QString::fromUtf8("ملفات اختصارات لسان (*.json);;كل الملفات (*)"));
        if (path.isEmpty()) {
            return;
        }
        if (!exportShortcutSettingsToPath(path)) {
            QMessageBox::warning(this, QString::fromUtf8("تعذر تصدير الاختصارات"), statusLabel->text());
        }
    });
    auto *shortcutResetButton = new QPushButton(QString::fromUtf8("استعادة الافتراضي"), shortcutButtonRow);
    shortcutResetButton->setObjectName(QStringLiteral("shortcutResetButton"));
    connect(shortcutResetButton, &QPushButton::clicked, this, [this]() {
        resetShortcutSettingsToDefaults();
    });
    shortcutButtonLayout->addWidget(shortcutImportButton);
    shortcutButtonLayout->addWidget(shortcutExportButton);
    shortcutButtonLayout->addWidget(shortcutResetButton);
    shortcutsLayout->addWidget(shortcutButtonRow);

    pages->addTab(editorPage, QString::fromUtf8("المحرر"));
    pages->addTab(runtimePage, QString::fromUtf8("التشغيل"));
    pages->addTab(projectsPage, QString::fromUtf8("المشاريع"));
    pages->addTab(shortcutsPage, QString::fromUtf8("الاختصارات"));
    categories->setCurrentRow(0);
    connect(categories, &QListWidget::currentRowChanged, pages, &QTabWidget::setCurrentIndex);

    contentLayout->addWidget(categories);
    contentLayout->addWidget(pages, 1);
    rootLayout->addLayout(contentLayout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->setLayoutDirection(Qt::RightToLeft);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("تطبيق"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("إلغاء"));
    rootLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    settings.setEditorFontFamily(fontFamilyCombo->currentText().trimmed().isEmpty()
        ? arabicEditorFontFamilies().first()
        : fontFamilyCombo->currentText().trimmed());
    settings.setEditorFontSize(fontSizeInput->value());
    settings.setThemePreference(themePreferenceCombo->currentData().toString());
    applyThemePreference();
    if (editorTabsController) {
        editorTabsController->applyFont(configuredEditorFont(settings));
    }
    setStatus(QString::fromUtf8("تم تحديث الإعدادات"));
}

void MainWindow::setStatus(const QString &text)
{
    statusLabel->setText(text);
}

bool MainWindow::loadProject(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return false;
    }

    const QString requestedRoot = QDir(path).absolutePath();
    if (!projectRoot.isEmpty()
        && QDir::cleanPath(projectRoot) != QDir::cleanPath(requestedRoot)
        && !confirmUnsavedDocuments(UnsavedChangesOperation::ProjectSwitch)) {
        return false;
    }

    projectRoot = requestedRoot;
    workspaceSettings = WorkspaceSettingsStore(projectRoot).load();
    if (projectTreeController) {
        projectTreeController->setProjectRoot(projectRoot);
    }
    if (editorTabsController) {
        editorTabsController->applyWorkspaceSettings(workspaceSettings);
    }
    settings.addRecentProject(projectRoot);
    setStatus(QString::fromUtf8("المشروع: %1").arg(projectRoot));
    return true;
}

bool MainWindow::openEditorFile(const QString &path)
{
    const QString normalizedPath = QFileInfo(path).absoluteFilePath();
    const DocumentId existingId = documentRegistry.findByPath(normalizedPath);
    if (editorTabsController && editorTabsController->surfaceForDocument(existingId)) {
        editorTabsController->openFile(normalizedPath);
        return true;
    }

    if (editor && editor->isDirty() && !confirmUnsavedDocuments(UnsavedChangesOperation::OpenFile)) {
        return false;
    }

    QString error;
    if (!editorTabsController || !editorTabsController->openFile(normalizedPath, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
        return false;
    }

    if (projectRoot.isEmpty()) {
        loadProject(QFileInfo(normalizedPath).absolutePath());
    }
    settings.addRecentFile(normalizedPath);
    setStatus(QString::fromUtf8("فتح: %1").arg(QFileInfo(normalizedPath).fileName()));
    return true;
}

bool MainWindow::requestCloseEditorTab(int index)
{
    if (!editorTabs || !editorTabsController || index < 0 || index >= editorTabs->count()) {
        return false;
    }

    auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(index));
    if (!surface) {
        return false;
    }

    EditorSurface *previousEditor = editor;
    editorTabs->setCurrentWidget(surface);
    if (!confirmSaveIfDirty()) {
        if (previousEditor) {
            editorTabs->setCurrentWidget(previousEditor);
        }
        return false;
    }

    editorTabsController->closeTab(index);
    return true;
}

void MainWindow::pollOpenDocumentChanges()
{
    const DocumentChangeSnapshot snapshot = documentChangePoller.poll();
    if (!snapshot.hasChanges()) {
        return;
    }

    setStatus(QString::fromUtf8("تغيرت ملفات مفتوحة خارج الاستوديو"));
    for (const DocumentRecord &record : snapshot.changedDocuments) {
        resolveExternalDocumentChange(record);
    }
    refreshEditorProblems();
}

void MainWindow::resolveExternalDocumentChange(const DocumentRecord &record)
{
    DocumentRecord current = documentRegistry.document(record.id);
    if (!current.id.isValid() || current.externalState == DocumentExternalState::Unchanged) {
        return;
    }

    EditorSurface *surface = editorTabsController ? editorTabsController->surfaceForDocument(current.id) : nullptr;
    const bool hasDirtyBuffer = (surface && surface->isDirty()) || current.dirty;

    QMessageBox box(this);
    box.setObjectName(QStringLiteral("externalChangeDialog"));
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QString::fromUtf8("تغير ملف مفتوح"));
    box.setText(current.externalState == DocumentExternalState::Deleted
            ? QString::fromUtf8("تم حذف ملف مفتوح خارج استوديو لسان.")
            : QString::fromUtf8("تغير ملف مفتوح خارج استوديو لسان."));
    box.setInformativeText(QString::fromUtf8("%1\n%2")
            .arg(QDir::toNativeSeparators(current.path))
            .arg(hasDirtyBuffer
                    ? QString::fromUtf8("لديك تغييرات غير محفوظة. الاحتفاظ بنسختك سيجعلها جاهزة للحفظ فوق النسخة الخارجية.")
                    : QString::fromUtf8("اختر إعادة تحميل نسخة القرص أو الاحتفاظ بالنسخة الحالية في المحرر.")));
    box.setLayoutDirection(Qt::RightToLeft);

    auto *reloadButton = box.addButton(QString::fromUtf8("إعادة تحميل"), QMessageBox::AcceptRole);
    reloadButton->setObjectName(QStringLiteral("externalChangeReloadButton"));
    reloadButton->setEnabled(current.externalState != DocumentExternalState::Deleted && !hasDirtyBuffer);
    auto *keepButton = box.addButton(QString::fromUtf8("الاحتفاظ بنسختي"), QMessageBox::RejectRole);
    keepButton->setObjectName(QStringLiteral("externalChangeKeepButton"));
    box.setDefaultButton(reloadButton->isEnabled() ? reloadButton : keepButton);
    box.exec();

    if (box.clickedButton() == reloadButton && reloadButton->isEnabled()) {
        QString error;
        if (!documentRegistry.reloadFromDisk(current.id, &error)) {
            QMessageBox::warning(this, QString::fromUtf8("تعذر إعادة التحميل"), error);
            return;
        }

        const DocumentRecord reloaded = documentRegistry.document(current.id);
        if (surface) {
            if (!surface->openFile(reloaded.path, &error)) {
                QMessageBox::warning(this, QString::fromUtf8("تعذر إعادة التحميل"), error);
                return;
            }
            if (editorTabsController) {
                editorTabsController->syncSessionsInto(workbenchState);
            }
            if (surface == editor) {
                updateBreadcrumbBar();
                updateStatusIndicators();
            }
        }
        setStatus(QString::fromUtf8("أعيد تحميل الملف من القرص"));
        return;
    }

    if (surface && documentRegistry.document(current.id).text != surface->toPlainText()) {
        documentRegistry.setText(current.id, surface->toPlainText());
    }
    documentRegistry.keepCurrentVersion(current.id);
    if (surface) {
        surface->document()->setModified(true);
        if (editorTabsController) {
            editorTabsController->syncSessionsInto(workbenchState);
        }
        if (surface == editor) {
            updateStatusIndicators();
        }
    }
    setStatus(QString::fromUtf8("تم الاحتفاظ بنسخة المحرر"));
}

void MainWindow::applyThemePreference()
{
    const QString preference = settings.themePreference();
    setProperty("themePreference", preference);
    setStyleSheet(preference == QStringLiteral("light")
            ? WorkbenchTheme::lightStyleSheet()
            : WorkbenchTheme::darkStyleSheet());
}

void MainWindow::applyShortcutSettings()
{
    const QJsonObject shortcutJson = settings.shortcutSettingsJson();
    ShortcutSettingsModel shortcutSettings;
    QString error;
    const bool hasShortcutSettings = !shortcutJson.isEmpty();
    if (hasShortcutSettings && !shortcutSettings.loadJson(shortcutJson, commandRegistry, &error)) {
        setProperty("shortcutSettingsError", error);
        return;
    }
    setProperty("shortcutSettingsError", QString());

    for (auto *action : findChildren<QAction *>()) {
        const QString commandId = action->property("commandId").toString();
        if (commandId.isEmpty() || !commandRegistry.contains(commandId)) {
            continue;
        }

        QKeySequence shortcut;
        if (hasShortcutSettings) {
            shortcut = shortcutSettings.effectiveShortcut(commandRegistry, commandId);
        } else {
            for (const CommandDefinition &command : commandRegistry.commands()) {
                if (command.id == commandId) {
                    shortcut = command.defaultShortcut;
                    break;
                }
            }
        }

        action->setShortcut(shortcut);
        if (!shortcut.isEmpty()) {
            action->setShortcutContext(Qt::ApplicationShortcut);
        }
        if (!shortcut.isEmpty() && !actions().contains(action)) {
            addAction(action);
        }
    }
}

void MainWindow::updateBreadcrumbBar()
{
    if (!breadcrumbPathLabel || !breadcrumbSymbolLabel) {
        return;
    }

    const QString path = editor ? editor->currentFilePath() : QString();
    breadcrumbPathLabel->setText(path.isEmpty()
        ? QString::fromUtf8("ملف جديد")
        : QDir::toNativeSeparators(path));
    breadcrumbSymbolLabel->setText(QString::fromUtf8("الرموز: لاحقا"));
}

void MainWindow::refreshCurrentEditorUi(bool includeProblems)
{
    if (!editor) {
        return;
    }

    setWindowTitle(editor->currentFilePath().isEmpty()
        ? QString::fromUtf8("استوديو لسان")
        : QString::fromUtf8("استوديو لسان - %1").arg(QFileInfo(editor->currentFilePath()).fileName()));
    updateBreadcrumbBar();
    updateStatusIndicators();
    if (includeProblems) {
        refreshEditorProblems();
    }
}

void MainWindow::configureLanguageServer()
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.remove(QStringLiteral("PYTHONHOME"));
    environment.remove(QStringLiteral("PYTHONPATH"));
    environment.insert(QStringLiteral("PYTHONNOUSERSITE"), QStringLiteral("1"));
    environment.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    environment.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));

    LspServerCommand command;
    command.program = runtimeOrchestrator.pythonExecutablePath();
    command.arguments = {QStringLiteral("-m"), QStringLiteral("arabicpython.lsp.server")};
    command.workingDirectory = projectRoot.isEmpty() ? QDir::homePath() : projectRoot;
    command.environment = environment;
    lspClient.setServerCommand(command);
}

void MainWindow::syncCurrentEditorToLanguageServer(bool reopenDocument)
{
    if (!editor) {
        closeLanguageServerDocument();
        return;
    }

    const QString path = editor->currentFilePath();
    if (path.isEmpty() || !isApySourcePath(path)) {
        closeLanguageServerDocument();
        return;
    }

    if (!QFileInfo::exists(runtimeOrchestrator.pythonExecutablePath())) {
        return;
    }

    if (!lspClient.isRunning()) {
        configureLanguageServer();
        const QString rootPath = projectRoot.isEmpty() ? QFileInfo(path).absolutePath() : projectRoot;
        QString error;
        if (!lspClient.startAndInitialize(QUrl::fromLocalFile(rootPath).toString(), 1000, &error)) {
            return;
        }
    }

    const QString uri = QUrl::fromLocalFile(path).toString();
    if (reopenDocument || !lspDocumentOpen || lspDocumentUri != uri) {
        closeLanguageServerDocument();
        lspDocumentUri = uri;
        lspDocumentVersion = 1;
        lspDocumentOpen = true;
        lspClient.openDocument(lspDocumentUri, QStringLiteral("apy"), lspDocumentVersion, editor->toPlainText());
        return;
    }

    ++lspDocumentVersion;
    lspClient.changeDocument(lspDocumentUri, lspDocumentVersion, editor->toPlainText());
}

void MainWindow::notifyLanguageServerOfSave()
{
    if (!editor) {
        return;
    }

    const QString path = editor->currentFilePath();
    const QString uri = QUrl::fromLocalFile(path).toString();
    if (lspClient.isRunning() && lspDocumentOpen && lspDocumentUri == uri) {
        lspClient.saveDocument(uri, editor->toPlainText());
        return;
    }

    syncCurrentEditorToLanguageServer(true);
}

void MainWindow::closeLanguageServerDocument()
{
    if (!lspDocumentOpen || lspDocumentUri.isEmpty() || !lspClient.isRunning()) {
        lspDocumentOpen = false;
        lspDocumentUri.clear();
        lspDocumentVersion = 0;
        return;
    }

    lspClient.closeDocument(lspDocumentUri);
    lspDocumentOpen = false;
    lspDocumentUri.clear();
    lspDocumentVersion = 0;
}

void MainWindow::requestLanguageServerCompletion(int line, int character)
{
    if (!editor) {
        return;
    }

    syncCurrentEditorToLanguageServer(false);
    if (!lspClient.isRunning() || !lspDocumentOpen || !lspClient.initializeResult().completionProvider) {
        return;
    }

    QString error;
    const QVector<LspCompletionItem> lspItems = lspClient.requestCompletion(lspDocumentUri, line, character, 200, &error);
    if (!error.isEmpty()) {
        return;
    }

    QVector<EditorCompletionItem> editorItems;
    editorItems.reserve(lspItems.size());
    for (const LspCompletionItem &item : lspItems) {
        editorItems.append({item.label, item.detail, item.insertText});
    }
    editor->showCompletionItems(editorItems);
}

void MainWindow::requestLanguageServerHover(int line, int character, const QPoint &viewportPosition)
{
    if (!editor) {
        return;
    }

    syncCurrentEditorToLanguageServer(false);
    if (!lspClient.isRunning() || !lspDocumentOpen || !lspClient.initializeResult().hoverProvider) {
        return;
    }

    QString error;
    const LspHoverResult hover = lspClient.requestHover(lspDocumentUri, line, character, 100, &error);
    if (error.isEmpty() && hover.hasContent) {
        editor->showHoverMarkdown(hover.markdown, viewportPosition);
    }
}

void MainWindow::requestLanguageServerDefinition(int line, int character)
{
    if (!editor) {
        return;
    }

    syncCurrentEditorToLanguageServer(false);
    if (!lspClient.isRunning() || !lspDocumentOpen) {
        return;
    }

    QString error;
    const QVector<LspLocation> locations = lspClient.requestDefinition(lspDocumentUri, line, character, 500, &error);
    if (!error.isEmpty() || locations.isEmpty()) {
        return;
    }

    const LspLocation location = locations.first();
    const QString path = QUrl(location.uri).toLocalFile();
    if (!path.isEmpty() && QFileInfo(path).isFile()) {
        openEditorFile(path);
    }
    goToEditorLocation(location.line + 1, location.character + 1);
}

void MainWindow::requestLanguageServerReferences()
{
    if (!editor) {
        return;
    }

    const QTextCursor cursor = editor->textCursor();
    syncCurrentEditorToLanguageServer(false);
    if (!lspClient.isRunning() || !lspDocumentOpen) {
        return;
    }

    QString error;
    const QVector<LspLocation> locations = lspClient.requestReferences(
        lspDocumentUri,
        cursor.blockNumber(),
        cursor.position() - cursor.block().position(),
        true,
        500,
        &error);
    if (error.isEmpty()) {
        renderReferences(locations);
    }
}

void MainWindow::requestLanguageServerRename()
{
    if (!editor) {
        return;
    }

    const QTextCursor cursor = editor->textCursor();
    bool accepted = false;
    const QString newName = QInputDialog::getText(
        this,
        QString::fromUtf8("إعادة تسمية الرمز"),
        QString::fromUtf8("الاسم الجديد:"),
        QLineEdit::Normal,
        cursor.selectedText(),
        &accepted);
    if (!accepted || newName.trimmed().isEmpty()) {
        return;
    }

    syncCurrentEditorToLanguageServer(false);
    if (!lspClient.isRunning() || !lspDocumentOpen) {
        setStatus(QString::fromUtf8("خادم اللغة غير جاهز لإعادة التسمية"));
        return;
    }

    QString error;
    const LspWorkspaceEdit edit = lspClient.requestRename(
        lspDocumentUri,
        cursor.blockNumber(),
        cursor.position() - cursor.block().position(),
        newName.trimmed(),
        1000,
        &error);
    if (!error.isEmpty()) {
        setStatus(error);
        return;
    }
    if (!applyWorkspaceEdit(edit, &error) && !error.isEmpty()) {
        setStatus(error);
    }
}

void MainWindow::updateStatusIndicators()
{
    if (!statusEncodingLabel || !statusLineEndingLabel || !statusIndentationLabel || !statusLanguageModeLabel || !statusRuntimeLabel || !statusGitLabel) {
        return;
    }

    const QString text = editor ? editor->toPlainText() : QString();
    const QString path = editor ? editor->currentFilePath() : QString();
    statusEncodingLabel->setText(QStringLiteral("UTF-8"));
    statusLineEndingLabel->setText(lineEndingStatusText(DocumentFileIO::detectLineEnding(text)));
    statusIndentationLabel->setText(QString::fromUtf8("مسافات: 4"));
    statusLanguageModeLabel->setText(languageModeStatusText(path));
    statusRuntimeLabel->setText(QString::fromUtf8("التشغيل: جاهز"));
    statusGitLabel->setText(QStringLiteral("Git: --"));
}

void MainWindow::writeOutput(const QString &title, const QString &text)
{
    showOutputPanel();
    outputTranscript.clear();
    outputTranscript.append(OutputTranscriptChannel::System, title, text);
    renderOutputTranscript();
}

void MainWindow::showOutputPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanels) {
        bottomPanels->showOutputPanel();
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showTerminalPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanels) {
        bottomPanels->showTerminalPanel();
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showProblemsPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanels) {
        bottomPanels->showProblemsPanel();
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showSearchResultsPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanels) {
        bottomPanels->showSearchResultsPanel();
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showReferencesPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanels) {
        bottomPanels->showReferencesPanel();
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::refreshEditorProblems()
{
    if (!problemsPanel || !editor) {
        return;
    }

    problemsPanel->clear();
    const QVector<HiddenBidiFinding> findings = editor->findHiddenBidiControls(editor->toPlainText());
    for (const auto &finding : findings) {
        const QTextBlock block = editor->document()->findBlock(finding.position);
        const int line = block.isValid() ? block.blockNumber() + 1 : 0;
        const int column = block.isValid() ? finding.position - block.position() + 1 : 0;
        addProblem(
            QString::fromUtf8("تحذير"),
            QString::fromUtf8("تحكم اتجاه مخفي: %1 عند السطر %2، العمود %3").arg(finding.unicodeName).arg(line).arg(column),
            editor->currentFilePath(),
            line);
    }
}

void MainWindow::addProblem(const QString &severity, const QString &message, const QString &path, int line)
{
    if (!problemsPanel) {
        return;
    }

    const QString fileName = path.isEmpty() ? QString::fromUtf8("المحرر الحالي") : QFileInfo(path).fileName();
    const int effectiveLine = line > 0 ? line : ((!path.isEmpty() && QFileInfo(path).exists()) ? 1 : 0);
    const QString lineText = effectiveLine > 0 ? QString::fromUtf8(" - السطر %1").arg(effectiveLine) : QString();
    auto *item = new QListWidgetItem(QStringLiteral("%1: %2%3 - %4").arg(severity, fileName, lineText, message), problemsPanel);
    item->setData(Qt::UserRole, path);
    item->setData(Qt::UserRole + 1, effectiveLine);
    item->setData(Qt::UserRole + 2, severity);
    item->setData(Qt::UserRole + 3, message);
    item->setToolTip(path.isEmpty()
        ? message
        : QStringLiteral("%1\n%2").arg(QDir::toNativeSeparators(path), message));
    item->setForeground(severity == QString::fromUtf8("خطأ") ? QColor(249, 112, 102) : QColor(123, 223, 242));
    item->setSizeHint(QSize(0, 74));

    auto *row = new QWidget(problemsPanel);
    row->setObjectName(QStringLiteral("problemRow"));
    row->setLayoutDirection(Qt::RightToLeft);
    auto *rowLayout = new QVBoxLayout(row);
    rowLayout->setContentsMargins(12, 8, 12, 8);
    rowLayout->setSpacing(4);

    auto *topLine = new QWidget(row);
    topLine->setLayoutDirection(Qt::RightToLeft);
    auto *topLayout = new QHBoxLayout(topLine);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    auto *severityLabel = new QLabel(severity, topLine);
    severityLabel->setObjectName(QStringLiteral("problemSeverityLabel"));
    severityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    severityLabel->setStyleSheet(severity == QString::fromUtf8("خطأ")
        ? QStringLiteral("color: #F97066; font-weight: 700;")
        : QStringLiteral("color: #7BDFF2; font-weight: 700;"));

    auto *locationLabel = new QLabel(QStringLiteral("%1%2").arg(fileName, lineText), topLine);
    locationLabel->setObjectName(QStringLiteral("problemLocationLabel"));
    locationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    locationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    locationLabel->setStyleSheet(QStringLiteral("color: #AEC6FF; font-weight: 600;"));

    topLayout->addWidget(severityLabel);
    topLayout->addWidget(locationLabel, 1);

    auto *messageLabel = new QLabel(message, row);
    messageLabel->setObjectName(QStringLiteral("problemMessageLabel"));
    messageLabel->setLayoutDirection(Qt::RightToLeft);
    messageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    messageLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    messageLabel->setWordWrap(false);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageLabel->setStyleSheet(QStringLiteral("color: #D6DEE9;"));

    auto *messageLine = new QWidget(row);
    messageLine->setObjectName(QStringLiteral("problemMessageLine"));
    messageLine->setLayoutDirection(Qt::RightToLeft);
    auto *messageLayout = new QHBoxLayout(messageLine);
    messageLayout->setDirection(QBoxLayout::RightToLeft);
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(0);
    messageLayout->addWidget(messageLabel);
    messageLayout->addStretch(1);

    rowLayout->addWidget(topLine);
    rowLayout->addWidget(messageLine);
    problemsPanel->setItemWidget(item, row);
}

void MainWindow::goToEditorLine(int line)
{
    goToEditorLocation(line, 1);
}

void MainWindow::goToEditorLocation(int line, int column)
{
    if (!editor || line < 1) {
        return;
    }

    const QTextBlock block = editor->document()->findBlockByNumber(line - 1);
    if (!block.isValid()) {
        return;
    }

    const int columnOffset = qBound(0, column - 1, qMax(0, block.length() - 1));
    QTextCursor cursor(editor->document());
    cursor.setPosition(block.position() + columnOffset);
    editor->setTextCursor(cursor);
    editor->centerCursor();
    editor->setFocus();
}

bool MainWindow::confirmSaveIfDirty()
{
    return confirmUnsavedDocuments(UnsavedChangesOperation::CloseDocument);
}

bool MainWindow::confirmHiddenBidiSave()
{
    if (!editor) {
        return true;
    }

    const QVector<HiddenBidiFinding> findings = editor->findHiddenBidiControls(editor->toPlainText());
    if (findings.isEmpty()) {
        return true;
    }

    QMessageBox box(this);
    box.setObjectName(QStringLiteral("hiddenBidiSaveWarningDialog"));
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QString::fromUtf8("محارف اتجاه مخفية"));
    box.setText(QString::fromUtf8("يحتوي الملف على محارف اتجاه مخفية قد تغير معنى الكود عند القراءة."));
    box.setInformativeText(QString::fromUtf8("عدد المحارف المخفية: %1\nاحذفها من لوحة المشاكل أو احفظ الملف على مسؤوليتك.")
            .arg(findings.size()));
    box.setLayoutDirection(Qt::RightToLeft);

    auto *saveAnyway = box.addButton(QString::fromUtf8("حفظ على أي حال"), QMessageBox::AcceptRole);
    saveAnyway->setObjectName(QStringLiteral("hiddenBidiSaveAnywayButton"));
    auto *cancel = box.addButton(QString::fromUtf8("إلغاء"), QMessageBox::RejectRole);
    cancel->setObjectName(QStringLiteral("hiddenBidiSaveCancelButton"));
    box.setDefaultButton(cancel);
    box.exec();

    return box.clickedButton() == saveAnyway;
}

bool MainWindow::confirmUnsavedDocuments(UnsavedChangesOperation operation)
{
    const UnsavedChangesRequest request = UnsavedChangesGuard::requestFor(documentRegistry.documents(), operation);
    if (!request.required) {
        return true;
    }

    const auto answer = QMessageBox::question(
        this,
        request.title,
        request.message,
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Discard) {
        discardUntitledDrafts();
        return true;
    }

    const QVector<EditorSurface *> surfaces = editorTabsController ? editorTabsController->dirtySurfaces() : QVector<EditorSurface *> {};
    for (EditorSurface *surface : surfaces) {
        const DocumentId id = editorTabsController->documentIdForSurface(surface);
        editorTabs->setCurrentWidget(surface);
        saveFile();
        if (documentRegistry.document(id).dirty) {
            return false;
        }
    }
    return true;
}

void MainWindow::discardUntitledDrafts()
{
    if (editorTabsController) {
        editorTabsController->discardUntitledDrafts();
    }
    saveWorkbenchSession();
}

QString MainWindow::runtimeWorkingDirectory() const
{
    if (!projectRoot.isEmpty()) {
        return projectRoot;
    }
    if (!editor->currentFilePath().isEmpty()) {
        return QFileInfo(editor->currentFilePath()).absolutePath();
    }
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

void MainWindow::runRuntimeAction(RuntimeAction action, const QString &title, bool reloadAfterSuccess)
{
    if (runtimeOrchestrator.isRunning()) {
        showOutputPanel();
        setStatus(QString::fromUtf8("هناك عملية قيد التشغيل"));
        return;
    }

    QString materializeError;
    const QString runFilePath = materializeRunnableBuffer(&materializeError);
    if (runFilePath.isEmpty()) {
        writeOutput(title, materializeError);
        return;
    }

    const QString workingDirectory = runtimeWorkingDirectory();
    showOutputPanel();
    const RuntimeLaunchPlan plan = runtimeOrchestrator.buildLaunchPlan(action, title, runFilePath, workingDirectory, reloadAfterSuccess);
    runtimeOrchestrator.start(plan, true);
}

void MainWindow::appendRuntimeOutput(OutputTranscriptChannel channel, const QString &label, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    showOutputPanel();
    outputTranscript.append(channel, label, text);
    renderOutputTranscript();
}

void MainWindow::setOutputFilter(const OutputTranscriptFilter &filter)
{
    outputFilter = filter;
    renderOutputTranscript();
    showOutputPanel();
}

void MainWindow::renderOutputTranscript()
{
    if (!outputPanel) {
        return;
    }

    outputPanel->setPlainText(outputTranscript.render(outputFilter));
}

void MainWindow::restoreWorkbenchSession(bool promptForDraftRecovery)
{
    SavedWorkbenchSession session = settings.savedWorkbenchSession();
    if (promptForDraftRecovery && !session.untitledDrafts.isEmpty()) {
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            QString::fromUtf8("استعادة المسودات"),
            QString::fromUtf8("تم العثور على مسودات غير محفوظة من جلسة لم تغلق بشكل طبيعي. هل تريد استعادتها؟"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (choice != QMessageBox::Yes) {
            session.untitledDrafts.clear();
            settings.saveWorkbenchSession(session);
        }
    }

    if (!session.projectRoot.isEmpty() && QFileInfo(session.projectRoot).isDir()) {
        loadProject(session.projectRoot);
    }

    bool openedAnyFile = false;
    for (const QString &path : session.openFiles) {
        if (path.isEmpty() || !QFileInfo(path).isFile()) {
            continue;
        }
        if (openEditorFile(path)) {
            openedAnyFile = true;
        }
    }

    bool restoredAnyDraft = false;
    for (const QString &draftText : session.untitledDrafts) {
        if (draftText.isEmpty()) {
            continue;
        }

        const bool canReuseInitialTab = !openedAnyFile
            && !restoredAnyDraft
            && editorTabs
            && editorTabs->count() == 1
            && editor
            && editor->currentFilePath().isEmpty()
            && !editor->isDirty()
            && editor->toPlainText().isEmpty();
        EditorSurface *target = canReuseInitialTab
            ? editor
            : (editorTabsController ? editorTabsController->createUntitled(QString::fromUtf8("مسودة مستعادة")) : nullptr);
        if (!target) {
            continue;
        }

        target->setPlainText(draftText);
        target->document()->setModified(true);
        restoredAnyDraft = true;
    }

    if ((openedAnyFile || restoredAnyDraft) && editorTabs) {
        const int activeIndex = qBound(0, session.activeFileIndex, editorTabs->count() - 1);
        editorTabs->setCurrentIndex(activeIndex);
    }

    if (bottomPanelTabs && bottomPanels) {
        bottomPanelTabs->setCurrentWidget(bottomPanels->panelForId(session.bottomPanelId));
    }
}

void MainWindow::saveWorkbenchSession()
{
    if (editorTabsController) {
        editorTabsController->syncSessionsInto(workbenchState);
    }

    SavedWorkbenchSession session;
    session.projectRoot = projectRoot;
    if (editorTabsController) {
        session.openFiles = editorTabsController->openFilePaths();
        session.untitledDrafts = editorTabsController->untitledDrafts();
        session.activeFileIndex = editorTabsController->activeFileIndexAmongSavedFiles();
    }
    if (session.activeFileIndex < 0 && !session.openFiles.isEmpty()) {
        session.activeFileIndex = 0;
    }
    session.bottomPanelId = bottomPanels && bottomPanelTabs
        ? bottomPanels->panelId(bottomPanelTabs->currentWidget())
        : QString();
    settings.saveWorkbenchSession(session);
}

bool MainWindow::hasDirtyUntitledDraft() const
{
    if (!editorTabs) {
        return false;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        const auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (surface && surface->currentFilePath().isEmpty() && surface->isDirty()) {
            return true;
        }
    }
    return false;
}

void MainWindow::scheduleUntitledDraftAutosave()
{
    if (!untitledDraftAutosaveTimer || !hasDirtyUntitledDraft() || untitledDraftAutosaveTimer->isActive()) {
        return;
    }
    untitledDraftAutosaveTimer->start();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsavedDocuments(UnsavedChangesOperation::Exit)) {
        event->ignore();
        return;
    }

    saveWorkbenchSession();
    settings.markWorkbenchSessionClosedGracefully();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (outputPanel && watched == outputPanel->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            outputPanel->setTextCursor(outputPanel->cursorForPosition(mouseEvent->pos()));
            if (openOutputLinkAtCursor()) {
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}
