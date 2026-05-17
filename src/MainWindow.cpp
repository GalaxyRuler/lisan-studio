#include "MainWindow.h"

#include "ApySnippetService.h"
#include "DocumentFileIO.h"
#include "ProjectFileOperations.h"
#include "RuntimeProblemParser.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
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
#include <QInputDialog>
#include <QIcon>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QSpinBox>
#include <QTabBar>
#include <QTextBlock>
#include <QTextOption>
#include <QTextStream>
#include <QToolButton>
#include <QStandardPaths>
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

static QLabel *createStatusIndicator(const QString &objectName, const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(objectName);
    label->setLayoutDirection(Qt::LeftToRight);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    label->setMinimumWidth(72);
    return label;
}

static QString lightWorkbenchStyleSheet()
{
    return QStringLiteral(
        "QMainWindow, QWidget { background: #F6F7FB; color: #17202A; font-family: 'IBM Plex Sans Arabic', 'Segoe UI'; }"
        "QFrame[role=\"menuPopup\"] { background: #FFFFFF; color: #17202A; border: 1px solid #CCD4E0; }"
        "QPushButton[role=\"menuRow\"] { background: transparent; border: 0; border-radius: 0; padding: 8px 18px; text-align: right; color: #17202A; }"
        "QPushButton[role=\"menuRow\"]:hover { background: #DCEBFF; color: #0C2D57; }"
        "QWidget#topShell { background: #FFFFFF; border-bottom: 1px solid #CCD4E0; }"
        "QWidget#topMenuRow { background: #FFFFFF; border-bottom: 1px solid #CCD4E0; }"
        "QWidget#brandBlock { background: transparent; }"
        "QLabel#brandTextLabel { color: #24589C; font-weight: 600; font-size: 18px; }"
        "QToolButton[role=\"topMenu\"] { background: transparent; border: 1px solid transparent; border-radius: 4px; color: #35465B; font-weight: 600; padding: 7px 10px; }"
        "QToolButton[role=\"topMenu\"]:hover { color: #17202A; background: #EEF4FF; border-color: #CCD4E0; border-bottom: 2px solid #3B74C5; }"
        "QToolButton[role=\"topMenu\"]:pressed, QToolButton[role=\"topMenu\"]:checked { color: #174A8B; background: #DCEBFF; border-color: #3B74C5; }"
        "QToolButton[role=\"topMenu\"][active=\"true\"] { color: #174A8B; border-bottom: 2px solid #3B74C5; }"
        "QToolButton[role=\"topMenu\"]::menu-indicator { image: none; width: 0px; }"
        "QToolButton[role=\"primaryAction\"] { background: #FFFFFF; border: 1px solid #3B74C5; border-radius: 12px; padding: 4px 12px; color: #145C72; font-weight: 700; }"
        "QToolButton[role=\"primaryAction\"]:hover { background: #E4F7FB; border-color: #20869E; }"
        "QToolButton { background: #FFFFFF; border: 1px solid #CCD4E0; border-radius: 4px; padding: 6px 10px; color: #17202A; }"
        "QToolButton:hover { border-color: #3B74C5; background: #EEF4FF; }"
        "QToolButton:disabled { color: #8996A8; background: #EEF1F5; }"
        "QLineEdit { background: #FFFFFF; border: 1px solid #CCD4E0; border-radius: 12px; padding: 5px 12px; color: #17202A; selection-background-color: #B9D5FF; }"
        "QLineEdit:focus { border-color: #3B74C5; }"
        "QTreeView, QPlainTextEdit, QListWidget { background: #FFFFFF; border: 1px solid #CCD4E0; selection-background-color: #B9D5FF; color: #17202A; }"
        "QListWidget::item { padding: 8px 10px; border-bottom: 1px solid #E2E7EF; }"
        "QListWidget::item:hover { background: #EEF4FF; color: #0C2D57; }"
        "QListWidget::item:selected { background: #B9D5FF; color: #0C2D57; }"
        "QTabWidget::pane { border: 1px solid #CCD4E0; background: #FFFFFF; }"
        "QTabBar::tab { background: #EEF1F5; color: #536274; border: 1px solid #CCD4E0; padding: 7px 12px; }"
        "QTabBar::tab:selected { background: #FFFFFF; color: #17202A; border-top: 2px solid #3B74C5; }"
        "QDockWidget::title { background: #FFFFFF; padding: 6px; text-align: right; color: #17202A; }"
        "QStatusBar { background: #FFFFFF; color: #536274; border-top: 1px solid #CCD4E0; }");
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
      settings(settingsPath)
{
    buildUi();
    restoreWorkbenchSession();
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
            QString saveError;
            if (!editor->saveFile(&saveError)) {
                if (error) {
                    *error = saveError;
                }
                return QString();
            }
        }
        return editor->currentFilePath();
    }

    const QString baseDirectory = runtimeWorkingDirectory();
    if (baseDirectory.isEmpty()) {
        if (error) {
            *error = QString::fromUtf8("تعذر تحديد مجلد التشغيل.");
        }
        return QString();
    }

    QDir runDirectory(baseDirectory);
    if (!runDirectory.mkpath(QStringLiteral(".arabic-code-studio"))) {
        if (error) {
            *error = QString::fromUtf8("تعذر إنشاء مجلد التشغيل المؤقت.");
        }
        return QString();
    }

    const QString runFilePath = runDirectory.filePath(QStringLiteral(".arabic-code-studio/current-buffer.apy"));
    QFile file(runFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return QString();
    }

    file.write(editor->toPlainText().toUtf8());
    return runFilePath;
}

void MainWindow::buildUi()
{
    setLayoutDirection(Qt::RightToLeft);
    registerWorkbenchCommands();

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background: #0f141a; color: #E8ECF2; font-family: 'IBM Plex Sans Arabic', 'Segoe UI'; }"
        "QFrame[role=\"menuPopup\"] { background: #171B22; color: #E8ECF2; border: 1px solid #303746; }"
        "QPushButton[role=\"menuRow\"] { background: transparent; border: 0; border-radius: 0; padding: 8px 18px; text-align: right; color: #E8ECF2; }"
        "QPushButton[role=\"menuRow\"]:hover { background: #264F78; color: #FFFFFF; }"
        "QWidget#topShell { background: #111318; border-bottom: 1px solid #303746; }"
        "QWidget#topMenuRow { background: #111318; border-bottom: 1px solid #303746; }"
        "QWidget#brandBlock { background: transparent; }"
        "QLabel#brandTextLabel { color: #AEC6FF; font-weight: 600; font-size: 18px; }"
        "QToolButton[role=\"topMenu\"] { background: transparent; border: 1px solid transparent; border-radius: 4px; color: #C8D0DD; font-weight: 600; padding: 7px 10px; }"
        "QToolButton[role=\"topMenu\"]:hover { color: #E8ECF2; background: #202633; border-color: #303746; border-bottom: 2px solid #4C8DFF; }"
        "QToolButton[role=\"topMenu\"]:pressed, QToolButton[role=\"topMenu\"]:checked { color: #A7C4FF; background: #13233A; border-color: #4C8DFF; }"
        "QToolButton[role=\"topMenu\"][active=\"true\"] { color: #A7C4FF; border-bottom: 2px solid #4C8DFF; }"
        "QToolButton[role=\"topMenu\"]::menu-indicator { image: none; width: 0px; }"
        "QToolButton[role=\"primaryAction\"] { background: transparent; border: 1px solid #4C8DFF; border-radius: 12px; padding: 4px 12px; color: #7BDFF2; font-weight: 700; }"
        "QToolButton[role=\"primaryAction\"]:hover { background: #13233A; border-color: #7BDFF2; }"
        "QToolButton { background: #202633; border: 1px solid #303746; border-radius: 4px; padding: 6px 10px; color: #E8ECF2; }"
        "QToolButton:hover { border-color: #4C8DFF; background: #262d3b; }"
        "QToolButton:disabled { color: #6F7A8A; background: #171B22; }"
        "QLineEdit { background: #1B2230; border: 1px solid #303746; border-radius: 12px; padding: 5px 12px; color: #E8ECF2; selection-background-color: #264F78; }"
        "QLineEdit:focus { border-color: #4C8DFF; }"
        "QTreeView, QPlainTextEdit, QListWidget { background: #0D1117; border: 1px solid #303746; selection-background-color: #264F78; color: #E8ECF2; }"
        "QListWidget::item { padding: 8px 10px; border-bottom: 1px solid #1B2230; }"
        "QListWidget::item:hover { background: #13233A; color: #FFFFFF; }"
        "QListWidget::item:selected { background: #264F78; color: #FFFFFF; }"
        "QTabWidget::pane { border: 1px solid #303746; background: #0D1117; }"
        "QTabBar::tab { background: #171B22; color: #A7B0BE; border: 1px solid #303746; padding: 7px 12px; }"
        "QTabBar::tab:selected { background: #202633; color: #E8ECF2; border-top: 2px solid #4C8DFF; }"
        "QDockWidget::title { background: #111318; padding: 6px; text-align: right; color: #E8ECF2; }"
        "QStatusBar { background: #111318; color: #A7B0BE; border-top: 1px solid #303746; }"));
    darkThemeStyleSheet = styleSheet();
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
    saveAction->setProperty("commandId", QStringLiteral("save-file"));
    auto *openProjectAction = makeAction(style()->standardIcon(QStyle::SP_DirOpenIcon), QString::fromUtf8("فتح مشروع"), &MainWindow::openFolder);
    openProjectAction->setProperty("commandId", QStringLiteral("open-project"));
    runAction = makeAction(lightPlayIcon(), QString::fromUtf8("تشغيل"), &MainWindow::runCurrentFile);
    runAction->setObjectName(QStringLiteral("runAction"));
    runAction->setProperty("commandId", QStringLiteral("run-current-file"));
    runAction->setShortcut(QKeySequence(QStringLiteral("F5")));
    runAction->setShortcutContext(Qt::ApplicationShortcut);
    cancelRunAction = makeAction(style()->standardIcon(QStyle::SP_MediaStop), QString::fromUtf8("إيقاف"), &MainWindow::cancelRuntimeProcess);
    cancelRunAction->setObjectName(QStringLiteral("cancelRunAction"));
    cancelRunAction->setProperty("commandId", QStringLiteral("stop-run"));
    cancelRunAction->setShortcut(QKeySequence(QStringLiteral("Shift+F5")));
    cancelRunAction->setShortcutContext(Qt::ApplicationShortcut);
    cancelRunAction->setEnabled(false);
    lintAction = makeAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), QString::fromUtf8("فحص"), &MainWindow::lintCurrentFile);
    lintAction->setObjectName(QStringLiteral("lintAction"));
    lintAction->setProperty("commandId", QStringLiteral("lint-current-file"));
    formatAction = makeAction(style()->standardIcon(QStyle::SP_BrowserReload), QString::fromUtf8("تنسيق"), &MainWindow::formatCurrentFile);
    formatAction->setObjectName(QStringLiteral("formatAction"));
    formatAction->setProperty("commandId", QStringLiteral("format-current-file"));
    commandPaletteAction = makeAction(style()->standardIcon(QStyle::SP_FileDialogListView), QString::fromUtf8("لوحة الأوامر"), &MainWindow::openCommandPalette);
    commandPaletteAction->setObjectName(QStringLiteral("commandPaletteAction"));
    commandPaletteAction->setProperty("commandId", QStringLiteral("command-palette"));
    commandPaletteAction->setShortcuts({QKeySequence(QStringLiteral("Ctrl+Shift+P"))});
    auto *settingsAction = makeAction(QIcon(), QString::fromUtf8("الإعدادات"), &MainWindow::openSettings);
    settingsAction->setObjectName(QStringLiteral("settingsAction"));
    settingsAction->setProperty("commandId", QStringLiteral("settings"));

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

    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("ملف جديد"), QKeySequence::New, QStringLiteral("new-file")), &QAction::triggered, this, &MainWindow::newFile);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("فتح ملف"), QKeySequence::Open, QStringLiteral("open-file")), &QAction::triggered, this, &MainWindow::openFile);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("فتح مشروع"), QKeySequence(), QStringLiteral("open-project")), &QAction::triggered, this, &MainWindow::openFolder);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("حفظ"), QKeySequence::Save, QStringLiteral("save-file")), &QAction::triggered, this, &MainWindow::saveFile);
    connect(addTextOnlyMenuAction(fileMenu, QString::fromUtf8("حفظ باسم"), QKeySequence::SaveAs, QStringLiteral("save-as")), &QAction::triggered, this, &MainWindow::saveFileAs);
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
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("بحث واستبدال"), QKeySequence::Find, QStringLiteral("find-in-file")), &QAction::triggered, this, &MainWindow::openInFileFind);
    connect(addTextOnlyMenuAction(editMenu, QString::fromUtf8("إدراج اطبع"), QKeySequence(), QStringLiteral("snippet.insertPrint")), &QAction::triggered, this, &MainWindow::insertPrintSnippet);
    commandPaletteAction->setIconVisibleInMenu(false);
    connect(addTextOnlyMenuAction(viewMenu, QString::fromUtf8("لوحة الأوامر"), QKeySequence(), QStringLiteral("command-palette")), &QAction::triggered, this, &MainWindow::openCommandPalette);
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
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("فحص"), QKeySequence(), QStringLiteral("lint-current-file")), &QAction::triggered, this, &MainWindow::lintCurrentFile);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("تنسيق"), QKeySequence(), QStringLiteral("format-current-file")), &QAction::triggered, this, &MainWindow::formatCurrentFile);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("الثقة بمساحة العمل"), QKeySequence(), QStringLiteral("workspace.trust")), &QAction::triggered, this, &MainWindow::trustCurrentWorkspace);
    settingsAction->setIconVisibleInMenu(false);
    connect(addTextOnlyMenuAction(toolsMenu, QString::fromUtf8("الإعدادات"), QKeySequence(), QStringLiteral("settings")), &QAction::triggered, this, &MainWindow::openSettings);
    connect(addTextOnlyMenuAction(helpMenu, QString::fromUtf8("عن استوديو لسان")), &QAction::triggered, this, [this]() {
        QMessageBox::information(this, QString::fromUtf8("عن استوديو لسان"), QString::fromUtf8("استوديو لسان\nبيئة عربية أصلية لملفات .apy"));
    });

    runtimeTimeoutTimer = new QTimer(this);
    runtimeTimeoutTimer->setObjectName(QStringLiteral("runtimeTimeoutTimer"));
    runtimeTimeoutTimer->setSingleShot(true);
    runtimeTimeoutTimer->setInterval(30000);
    connect(runtimeTimeoutTimer, &QTimer::timeout, this, &MainWindow::handleRuntimeTimeout);

    commandBox = new QLineEdit(this);
    commandBox->setObjectName(QStringLiteral("commandBox"));
    commandBox->setProperty("commandId", QStringLiteral("search-project"));
    commandBox->setPlaceholderText(QString::fromUtf8("ابحث في الأوامر والملفات..."));
    commandBox->setLayoutDirection(Qt::RightToLeft);
    commandBox->setFixedWidth(440);
    connect(commandBox, &QLineEdit::returnPressed, this, &MainWindow::findInProject);

    projectReplaceInput = new QLineEdit(this);
    projectReplaceInput->setObjectName(QStringLiteral("projectReplaceInput"));
    projectReplaceInput->setProperty("commandId", QStringLiteral("replace-in-project"));
    projectReplaceInput->setPlaceholderText(QString::fromUtf8("استبدال بـ..."));
    projectReplaceInput->setLayoutDirection(Qt::RightToLeft);
    projectReplaceInput->setFixedWidth(180);

    projectReplacePreviewButton = new QPushButton(QString::fromUtf8("معاينة"), this);
    projectReplacePreviewButton->setObjectName(QStringLiteral("projectReplacePreviewButton"));
    projectReplacePreviewButton->setProperty("commandId", QStringLiteral("replace-in-project"));
    connect(projectReplacePreviewButton, &QPushButton::clicked, this, &MainWindow::previewProjectReplace);

    projectReplaceApplyButton = new QPushButton(QString::fromUtf8("تطبيق"), this);
    projectReplaceApplyButton->setObjectName(QStringLiteral("projectReplaceApplyButton"));
    projectReplaceApplyButton->setProperty("commandId", QStringLiteral("replace-in-project.applyAccepted"));
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
    projectTree->setHeaderHidden(true);
    projectTree->setModel(fileSystemModel);
    projectTree->setLayoutDirection(Qt::RightToLeft);
    projectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    projectTree->hideColumn(1);
    projectTree->hideColumn(2);
    projectTree->hideColumn(3);
    projectTree->setMinimumWidth(240);
    connect(projectTree, &QTreeView::doubleClicked, this, &MainWindow::openSelectedProjectFile);
    connect(projectTree, &QTreeView::customContextMenuRequested, this, &MainWindow::showProjectTreeContextMenu);

    projectTreeNewFileAction = new QAction(QString::fromUtf8("ملف جديد"), this);
    projectTreeNewFileAction->setObjectName(QStringLiteral("projectTreeNewFileAction"));
    projectTreeNewFileAction->setProperty("commandId", QStringLiteral("project.file.new"));
    connect(projectTreeNewFileAction, &QAction::triggered, this, &MainWindow::createProjectTreeFile);

    projectTreeNewFolderAction = new QAction(QString::fromUtf8("مجلد جديد"), this);
    projectTreeNewFolderAction->setObjectName(QStringLiteral("projectTreeNewFolderAction"));
    projectTreeNewFolderAction->setProperty("commandId", QStringLiteral("project.folder.new"));
    connect(projectTreeNewFolderAction, &QAction::triggered, this, &MainWindow::createProjectTreeFolder);

    projectTreeOpenAction = new QAction(QString::fromUtf8("فتح"), this);
    projectTreeOpenAction->setObjectName(QStringLiteral("projectTreeOpenAction"));
    projectTreeOpenAction->setProperty("commandId", QStringLiteral("project.item.open"));
    connect(projectTreeOpenAction, &QAction::triggered, this, &MainWindow::openProjectTreeItem);

    projectTreeRenameAction = new QAction(QString::fromUtf8("إعادة تسمية"), this);
    projectTreeRenameAction->setObjectName(QStringLiteral("projectTreeRenameAction"));
    projectTreeRenameAction->setProperty("commandId", QStringLiteral("project.item.rename"));
    connect(projectTreeRenameAction, &QAction::triggered, this, &MainWindow::renameProjectTreeItem);

    projectTreeDeleteAction = new QAction(QString::fromUtf8("حذف"), this);
    projectTreeDeleteAction->setObjectName(QStringLiteral("projectTreeDeleteAction"));
    projectTreeDeleteAction->setProperty("commandId", QStringLiteral("project.item.deleteWithPrompt"));
    connect(projectTreeDeleteAction, &QAction::triggered, this, &MainWindow::deleteProjectTreeItem);

    projectTreeRevealAction = new QAction(QString::fromUtf8("إظهار في مستكشف الملفات"), this);
    projectTreeRevealAction->setObjectName(QStringLiteral("projectTreeRevealAction"));
    projectTreeRevealAction->setProperty("commandId", QStringLiteral("project.item.reveal"));
    connect(projectTreeRevealAction, &QAction::triggered, this, &MainWindow::revealProjectTreeItem);

    projectTreeCopyPathAction = new QAction(QString::fromUtf8("نسخ المسار"), this);
    projectTreeCopyPathAction->setObjectName(QStringLiteral("projectTreeCopyPathAction"));
    projectTreeCopyPathAction->setProperty("commandId", QStringLiteral("project.item.copyPath"));
    connect(projectTreeCopyPathAction, &QAction::triggered, this, &MainWindow::copyProjectTreeItemPath);

    projectTreeOpenContainingFolderAction = new QAction(QString::fromUtf8("فتح المجلد الحاوي"), this);
    projectTreeOpenContainingFolderAction->setObjectName(QStringLiteral("projectTreeOpenContainingFolderAction"));
    projectTreeOpenContainingFolderAction->setProperty("commandId", QStringLiteral("project.item.openContainingFolder"));
    connect(projectTreeOpenContainingFolderAction, &QAction::triggered, this, &MainWindow::openProjectTreeContainingFolder);

    projectTreeRefreshAction = new QAction(QString::fromUtf8("تحديث"), this);
    projectTreeRefreshAction->setObjectName(QStringLiteral("projectTreeRefreshAction"));
    projectTreeRefreshAction->setProperty("commandId", QStringLiteral("project.refresh"));
    connect(projectTreeRefreshAction, &QAction::triggered, this, &MainWindow::refreshProjectTree);

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
    connect(editorTabs, &QTabWidget::currentChanged, this, [this](int index) {
        workbenchState.setCurrentEditorSessionIndex(index);
        setCurrentEditor(qobject_cast<EditorSurface *>(editorTabs->widget(index)));
    });
    connect(editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeEditorTab);
    createEditorTab(QString::fromUtf8("ملف جديد"));
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
    bottomPanelTabs->addTab(debugPanel, QString::fromUtf8("التصحيح"));

    outputDock = new QDockWidget(QString::fromUtf8("اللوحة السفلية"), this);
    outputDock->setObjectName(QStringLiteral("outputDock"));
    outputDock->setWidget(bottomPanelTabs);
    outputDock->setMinimumHeight(180);
    outputDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, outputDock);
    resizeDocks({outputDock}, {190}, Qt::Vertical);

    statusLabel = new QLabel(QString::fromUtf8("جاهز"), this);
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
    updateStatusIndicators();
}

void MainWindow::newFile()
{
    if (!confirmSaveIfDirty()) {
        return;
    }
    createEditorTab(QString::fromUtf8("ملف جديد"));
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
    QString error;
    if (!editor->saveFile(&error)) {
        saveFileAs();
        return;
    }
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

    QString error;
    if (!editor->saveFileAs(path, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر الحفظ"), error);
        return;
    }
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
    if (activeRuntimeProcess && activeRuntimeProcess->state() != QProcess::NotRunning) {
        showOutputPanel();
        setStatus(QString::fromUtf8("هناك عملية قيد التشغيل"));
        return;
    }

    const QVector<RuntimeHistoryEntry> entries = runtimeHistory.entries();
    if (entries.isEmpty()) {
        showOutputPanel();
        setStatus(QString::fromUtf8("لا يوجد تشغيل سابق"));
        return;
    }

    const RuntimeHistoryEntry lastRun = entries.first();
    const RuntimeLaunchPlan plan = runtime.buildLaunchPlan(
        lastRun.action,
        lastRun.title,
        lastRun.filePath,
        lastRun.workingDirectory,
        lastRun.reloadAfterSuccess);
    startRuntimeLaunchPlan(plan, true);
}

void MainWindow::cancelRuntimeProcess()
{
    if (!activeRuntimeProcess || activeRuntimeProcess->state() == QProcess::NotRunning) {
        return;
    }

    appendRuntimeOutput(QString::fromUtf8("النظام"), QString::fromUtf8("تم طلب إيقاف العملية."));
    activeRuntimeProcess->kill();
    setStatus(QString::fromUtf8("تم إيقاف التشغيل"));
}

void MainWindow::appendRuntimeStdout()
{
    if (!activeRuntimeProcess) {
        return;
    }
    const QString text = QString::fromUtf8(activeRuntimeProcess->readAllStandardOutput());
    activeRuntimeStdout.append(text);
    appendRuntimeOutput(QString::fromUtf8("stdout"), text);
}

void MainWindow::appendRuntimeStderr()
{
    if (!activeRuntimeProcess) {
        return;
    }
    const QString text = QString::fromUtf8(activeRuntimeProcess->readAllStandardError());
    activeRuntimeStderr.append(text);
    appendRuntimeOutput(QString::fromUtf8("stderr"), text);
}

void MainWindow::finishRuntimeProcess(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    if (!activeRuntimeProcess) {
        return;
    }

    appendRuntimeStdout();
    appendRuntimeStderr();
    const QString finalText = QString::fromUtf8("رمز الخروج: %1\nالمدة: %2 ms")
        .arg(exitCode)
        .arg(activeRuntimeTimer.isValid() ? activeRuntimeTimer.elapsed() : 0);
    appendRuntimeOutput(QString::fromUtf8("النظام"), finalText);
    if (exitCode != 0) {
        const RuntimeProblemDetail detail = parseRuntimeProblemDetail(
            activeRuntimeStderr.isEmpty() ? activeRuntimeStdout : activeRuntimeStderr,
            activeRuntimeTitle,
            exitCode);
        addProblem(
            QString::fromUtf8("خطأ"),
            detail.message,
            activeRuntimeProcess->property("runFilePath").toString(),
            detail.line);
        showProblemsPanel();
    }
    completeRuntimeProcess(QString::fromUtf8("%1 انتهى: %2").arg(activeRuntimeTitle).arg(exitCode));
}

void MainWindow::handleRuntimeProcessError(QProcess::ProcessError error)
{
    if (!activeRuntimeProcess || activeRuntimeHandledError) {
        return;
    }

    if (error != QProcess::FailedToStart) {
        return;
    }

    activeRuntimeHandledError = true;
    appendRuntimeOutput(
        QString::fromUtf8("النظام"),
        QString::fromUtf8("تعذر بدء العملية: %1\nرمز الخروج: -1").arg(activeRuntimeProcess->errorString()));
    addProblem(
        QString::fromUtf8("خطأ"),
        QString::fromUtf8("تعذر بدء %1: %2").arg(activeRuntimeTitle, activeRuntimeProcess->errorString()),
        activeRuntimeProcess->property("runFilePath").toString(),
        0);
    completeRuntimeProcess(QString::fromUtf8("تعذر بدء %1").arg(activeRuntimeTitle));
}

void MainWindow::handleRuntimeTimeout()
{
    if (!activeRuntimeProcess || activeRuntimeProcess->state() == QProcess::NotRunning) {
        return;
    }

    appendRuntimeOutput(QString::fromUtf8("النظام"), QString::fromUtf8("انتهت مهلة التشغيل."));
    addProblem(
        QString::fromUtf8("خطأ"),
        QString::fromUtf8("انتهت مهلة %1 بعد 30 ثانية.").arg(activeRuntimeTitle),
        activeRuntimeProcess->property("runFilePath").toString(),
        0);
    activeRuntimeProcess->kill();
    setStatus(QString::fromUtf8("انتهت مهلة التشغيل"));
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

    auto addCommandRow = [&](const CommandDefinition &command) {
        auto *item = new QListWidgetItem(commands);
        item->setData(Qt::UserRole, command.id);
        item->setData(Qt::UserRole + 1, command.title);
        item->setData(Qt::UserRole + 2, command.defaultShortcut.toString(QKeySequence::NativeText));
        item->setData(Qt::UserRole + 3, command.keywords);
        item->setSizeHint(QSize(0, 44));

        auto *row = new QWidget(commands);
        row->setLayoutDirection(Qt::LeftToRight);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 6, 12, 6);
        rowLayout->setSpacing(12);

        auto *shortcut = new QLabel(command.defaultShortcut.toString(QKeySequence::NativeText), row);
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

    for (const CommandDefinition &command : commandRegistry.commands()) {
        addCommandRow(command);
    }

    layout->addWidget(input);
    layout->addWidget(commands);

    QPointer<QDialog> dialogPointer(&dialog);
    std::function<void()> selectedCommand;

    auto normalized = [](QString text) {
        return text.trimmed().toCaseFolded();
    };

    auto firstVisibleRow = [&]() {
        for (int row = 0; row < commands->count(); ++row) {
            if (!commands->item(row)->isHidden()) {
                return row;
            }
        }
        return -1;
    };

    auto updateFilter = [&]() {
        const QString query = normalized(input->text());
        for (int row = 0; row < commands->count(); ++row) {
            auto *item = commands->item(row);
            const QString haystack = normalized(
                item->data(Qt::UserRole + 1).toString()
                + QLatin1Char(' ')
                + item->data(Qt::UserRole + 2).toString()
                + QLatin1Char(' ')
                + item->data(Qt::UserRole + 3).toString());
            item->setHidden(!query.isEmpty() && !haystack.contains(query));
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
        QStringLiteral("new-file"),
        QString::fromUtf8("ملف جديد"),
        QString::fromUtf8("ملف"),
        QKeySequence::New,
        QString::fromUtf8("ملف جديد new file"),
        [this]() { newFile(); });
    registerCommand(
        QStringLiteral("open-file"),
        QString::fromUtf8("فتح ملف"),
        QString::fromUtf8("ملف"),
        QKeySequence::Open,
        QString::fromUtf8("فتح ملف open file"),
        [this]() { openFile(); });
    registerCommand(
        QStringLiteral("open-project"),
        QString::fromUtf8("فتح مشروع"),
        QString::fromUtf8("ملف"),
        QKeySequence(),
        QString::fromUtf8("فتح مشروع مجلد open project folder"),
        [this]() { openFolder(); });
    registerCommand(
        QStringLiteral("save-file"),
        QString::fromUtf8("حفظ"),
        QString::fromUtf8("ملف"),
        QKeySequence::Save,
        QString::fromUtf8("حفظ save"),
        [this]() { saveFile(); });
    registerCommand(
        QStringLiteral("save-as"),
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
            for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
                auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
                if (!surface || !surface->isDirty()) {
                    continue;
                }
                setCurrentEditor(surface);
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
            syncEditorSession(editor);
            updateEditorTabTitle(editor);
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
                closeEditorTab(editorTabs->currentIndex());
            }
        },
        [this]() { return editorTabs && editorTabs->count() > 0; });
    registerCommand(
        QStringLiteral("find-in-file"),
        QString::fromUtf8("بحث واستبدال في الملف"),
        QString::fromUtf8("تحرير"),
        QKeySequence::Find,
        QString::fromUtf8("بحث استبدال في الملف find replace"),
        [this]() { openInFileFind(); },
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
        QStringLiteral("run-current-file"),
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
        [this]() { return !runtimeHistory.entries().isEmpty(); });
    registerCommand(
        QStringLiteral("lint-current-file"),
        QString::fromUtf8("فحص الملف الحالي"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(),
        QString::fromUtf8("فحص lint current file"),
        [this]() { lintCurrentFile(); });
    registerCommand(
        QStringLiteral("format-current-file"),
        QString::fromUtf8("تنسيق الملف الحالي"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(),
        QString::fromUtf8("تنسيق format current file"),
        [this]() { formatCurrentFile(); });
    registerCommand(
        QStringLiteral("stop-run"),
        QString::fromUtf8("إيقاف التشغيل"),
        QString::fromUtf8("تشغيل"),
        QKeySequence(QStringLiteral("Shift+F5")),
        QString::fromUtf8("إيقاف التشغيل stop cancel run"),
        [this]() { cancelRuntimeProcess(); },
        [this]() { return activeRuntimeProcess && activeRuntimeProcess->state() != QProcess::NotRunning; });
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
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("search-project"),
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
        QStringLiteral("replace-in-project"),
        QString::fromUtf8("معاينة الاستبدال في المشروع"),
        QString::fromUtf8("بحث"),
        QKeySequence(),
        QString::fromUtf8("معاينة استبدال في المشروع project replace preview"),
        [this]() { previewProjectReplace(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("replace-in-project.applyAccepted"),
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
        [this]() { createProjectTreeFile(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("project.folder.new"),
        QString::fromUtf8("مجلد جديد في المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("مجلد جديد في المشروع project new folder"),
        [this]() { createProjectTreeFolder(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.open"),
        QString::fromUtf8("فتح عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("فتح عنصر المشروع project open item"),
        [this]() { openProjectTreeItem(); },
        [this]() { return !activeProjectTreePath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.rename"),
        QString::fromUtf8("إعادة تسمية عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("إعادة تسمية rename project item"),
        [this]() { renameProjectTreeItem(); },
        [this]() { return !activeProjectTreePath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.deleteWithPrompt"),
        QString::fromUtf8("حذف عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("حذف مع تأكيد delete project item"),
        [this]() { deleteProjectTreeItem(); },
        [this]() { return !activeProjectTreePath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.reveal"),
        QString::fromUtf8("إظهار عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("إظهار في مستكشف الملفات reveal explorer"),
        [this]() { revealProjectTreeItem(); },
        [this]() { return !activeProjectTreePath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.copyPath"),
        QString::fromUtf8("نسخ مسار عنصر المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("نسخ المسار copy path"),
        [this]() { copyProjectTreeItemPath(); },
        [this]() { return !activeProjectTreePath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.item.openContainingFolder"),
        QString::fromUtf8("فتح المجلد الحاوي"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("فتح المجلد الحاوي open containing folder"),
        [this]() { openProjectTreeContainingFolder(); },
        [this]() { return !activeProjectTreePath().isEmpty(); });
    registerCommand(
        QStringLiteral("project.refresh"),
        QString::fromUtf8("تحديث المشروع"),
        QString::fromUtf8("المشروع"),
        QKeySequence(),
        QString::fromUtf8("تحديث المشروع refresh project"),
        [this]() { refreshProjectTree(); },
        [this]() { return !projectRoot.isEmpty(); });
    registerCommand(
        QStringLiteral("settings"),
        QString::fromUtf8("الإعدادات"),
        QString::fromUtf8("النظام"),
        QKeySequence(QStringLiteral("Ctrl+,")),
        QString::fromUtf8("الإعدادات settings preferences"),
        [this]() { openSettings(); });
    registerCommand(
        QStringLiteral("command-palette"),
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

    workspaceSettings.trusted = true;
    QString error;
    if (!WorkspaceSettingsStore(projectRoot).save(workspaceSettings, &error)) {
        setStatus(error);
        return;
    }

    setStatus(QString::fromUtf8("تمت الثقة بمساحة العمل"));
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
    const int generation = workbenchState.nextSearchGeneration();
    const QVector<SearchResultRow> immediateRows = currentEditorSearchResults(query);
    renderSearchResults(immediateRows);
    showSearchResultsPanel();
    setStatus(immediateRows.isEmpty()
        ? QString::fromUtf8("جار البحث عن: %1").arg(query)
        : QString::fromUtf8("نتائج فورية: %1، جار البحث في المشروع").arg(immediateRows.size()));

    auto *watcher = new QFutureWatcher<QVector<SearchResultRow>>(this);
    activeSearchWatcher = watcher;
    connect(watcher, &QFutureWatcher<QVector<SearchResultRow>>::finished, this, [this, watcher, generation, immediateRows]() {
        const QVector<SearchResultRow> rows = watcher->result();
        watcher->deleteLater();
        if (activeSearchWatcher == watcher) {
            activeSearchWatcher = nullptr;
        }
        if (!workbenchState.isCurrentSearchGeneration(generation)) {
            return;
        }
        const QVector<SearchResultRow> mergedRows = SearchService::mergeRows(immediateRows, rows);
        renderSearchResults(mergedRows);
        setStatus(QString::fromUtf8("نتائج البحث: %1").arg(mergedRows.size()));
    });
    watcher->setFuture(QtConcurrent::run([root, query]() {
        SearchService service;
        return service.search(root, query);
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
    const ProjectReplacePreview projectPreview = service.previewProject(projectRoot, query, replacement);
    const QVector<ProjectReplacePreviewRow> rows = ProjectReplaceService::mergePreviewRows(immediateRows, projectPreview.rows);

    renderProjectReplacePreview(rows);
    showSearchResultsPanel();
    setStatus(QString::fromUtf8("معاينة الاستبدال: %1").arg(rows.size()));
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
    for (int tabIndex = 0; editorTabs && tabIndex < editorTabs->count(); ++tabIndex) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(tabIndex));
        if (!surface || !surface->isDirty() || surface->currentFilePath().isEmpty()) {
            continue;
        }
        const QString openPath = QFileInfo(surface->currentFilePath()).absoluteFilePath();
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

void MainWindow::openSelectedProjectFile(const QModelIndex &index)
{
    const QString path = fileSystemModel->filePath(index);
    if (QFileInfo(path).isFile()) {
        openEditorFile(path);
    }
}

void MainWindow::showProjectTreeContextMenu(const QPoint &pos)
{
    if (!projectTree || !fileSystemModel) {
        return;
    }

    const QModelIndex index = projectTree->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    projectTreeContextIndex = index.siblingAtColumn(0);
    const QFileInfo info(fileSystemModel->filePath(projectTreeContextIndex));
    if (!info.exists()) {
        projectTreeContextIndex = QModelIndex();
        return;
    }

    QMenu menu(this);
    menu.setLayoutDirection(Qt::RightToLeft);
    menu.setObjectName(QStringLiteral("projectTreeContextMenu"));

    if (info.isDir()) {
        menu.addAction(projectTreeNewFileAction);
        menu.addAction(projectTreeNewFolderAction);
        menu.addSeparator();
        menu.addAction(projectTreeCopyPathAction);
        menu.addAction(projectTreeOpenContainingFolderAction);
        menu.addAction(projectTreeRevealAction);
        menu.addAction(projectTreeRefreshAction);
    } else {
        menu.addAction(projectTreeOpenAction);
        menu.addAction(projectTreeRenameAction);
        menu.addAction(projectTreeDeleteAction);
        menu.addSeparator();
        menu.addAction(projectTreeCopyPathAction);
        menu.addAction(projectTreeOpenContainingFolderAction);
        menu.addAction(projectTreeRevealAction);
    }

    menu.exec(projectTree->viewport()->mapToGlobal(pos));
    projectTreeContextIndex = QModelIndex();
}

void MainWindow::createProjectTreeFile()
{
    const QString folderPath = activeProjectTreeFolderPath();
    if (folderPath.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
        this,
        QString::fromUtf8("ملف جديد"),
        QString::fromUtf8("اسم الملف"),
        QLineEdit::Normal,
        QStringLiteral("main.apy"),
        &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }
    QString error;
    const ProjectFileOperationTarget target = ProjectFileOperations::childTarget(
        folderPath,
        projectRoot,
        name,
        true,
        &error);
    if (!target.allowed) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر إنشاء الملف"), error);
        return;
    }

    QFile file(target.path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر إنشاء الملف"), file.errorString());
        return;
    }
    file.close();

    refreshProjectTree();
    openEditorFile(target.path);
}

void MainWindow::createProjectTreeFolder()
{
    const QString folderPath = activeProjectTreeFolderPath();
    if (folderPath.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
        this,
        QString::fromUtf8("مجلد جديد"),
        QString::fromUtf8("اسم المجلد"),
        QLineEdit::Normal,
        QString::fromUtf8("مجلد جديد"),
        &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }
    QString error;
    const ProjectFileOperationTarget target = ProjectFileOperations::childTarget(
        folderPath,
        projectRoot,
        name,
        false,
        &error);
    if (!target.allowed) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر إنشاء المجلد"), error);
        return;
    }

    if (!QDir().mkpath(target.path)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر إنشاء المجلد"), QString::fromUtf8("راجع الاسم أو أذونات المجلد."));
        return;
    }
    refreshProjectTree();
}

void MainWindow::openProjectTreeItem()
{
    const QString path = activeProjectTreePath();
    if (path.isEmpty()) {
        return;
    }
    const QFileInfo info(path);
    if (info.isDir() && projectTree) {
        const QModelIndex index = activeProjectTreeIndex();
        projectTree->setExpanded(index, !projectTree->isExpanded(index));
        return;
    }
    if (info.isFile()) {
        openEditorFile(path);
    }
}

void MainWindow::renameProjectTreeItem()
{
    const QString path = activeProjectTreePath();
    const QFileInfo info(path);
    if (!info.exists() || info.absoluteFilePath() == QFileInfo(projectRoot).absoluteFilePath()) {
        return;
    }

    bool accepted = false;
    const QString newName = QInputDialog::getText(
        this,
        QString::fromUtf8("إعادة تسمية"),
        QString::fromUtf8("الاسم الجديد"),
        QLineEdit::Normal,
        info.fileName(),
        &accepted).trimmed();
    if (!accepted || newName.isEmpty() || newName == info.fileName()) {
        return;
    }
    QString error;
    const ProjectFileOperationTarget target = ProjectFileOperations::renameTarget(
        info.absoluteFilePath(),
        newName,
        projectRoot,
        &error);
    if (!target.allowed) {
        QMessageBox::warning(this, QString::fromUtf8("تعذرت إعادة التسمية"), error);
        return;
    }

    QDir parent(info.absolutePath());
    if (!parent.rename(info.fileName(), QFileInfo(target.path).fileName())) {
        QMessageBox::warning(this, QString::fromUtf8("تعذرت إعادة التسمية"), QString::fromUtf8("راجع أذونات الملف أو المجلد."));
        return;
    }

    refreshProjectTree();
    if (info.isFile() && QFileInfo(currentEditorPath()).absoluteFilePath() == info.absoluteFilePath()) {
        openEditorFile(target.path);
    }
}

void MainWindow::deleteProjectTreeItem()
{
    const QString path = activeProjectTreePath();
    const QFileInfo info(path);
    if (!info.exists()) {
        return;
    }

    QString error;
    if (!ProjectFileOperations::canDelete(info.absoluteFilePath(), projectRoot, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر الحذف"), error);
        return;
    }

    QMessageBox box(this);
    box.setLayoutDirection(Qt::RightToLeft);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QString::fromUtf8("تأكيد الحذف"));
    box.setText(info.isDir()
        ? QString::fromUtf8("هل تريد حذف هذا المجلد وكل محتوياته؟")
        : QString::fromUtf8("هل تريد حذف هذا الملف؟"));
    box.setInformativeText(QDir::toNativeSeparators(info.absoluteFilePath()));
    auto *deleteButton = box.addButton(QString::fromUtf8("حذف"), QMessageBox::AcceptRole);
    box.addButton(QString::fromUtf8("إلغاء"), QMessageBox::RejectRole);
    box.setDefaultButton(qobject_cast<QPushButton *>(box.buttons().last()));
    box.exec();
    if (box.clickedButton() != deleteButton) {
        return;
    }

    bool removed = false;
    if (info.isDir()) {
        removed = QDir(info.absoluteFilePath()).removeRecursively();
    } else {
        removed = QFile::remove(info.absoluteFilePath());
    }
    if (!removed) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر الحذف"), QString::fromUtf8("راجع أذونات الملف أو المجلد."));
        return;
    }

    clearEditorsForDeletedPath(info.absoluteFilePath());
    refreshProjectTree();
}

void MainWindow::revealProjectTreeItem()
{
    const QString path = activeProjectTreePath();
    if (path.isEmpty()) {
        return;
    }

    const QFileInfo info(path);
    QStringList arguments;
    if (info.isFile()) {
        arguments << QStringLiteral("/select,") << QDir::toNativeSeparators(info.absoluteFilePath());
    } else {
        arguments << QDir::toNativeSeparators(info.absoluteFilePath());
    }
    QProcess::startDetached(QStringLiteral("explorer.exe"), arguments);
}

void MainWindow::copyProjectTreeItemPath()
{
    const QString path = activeProjectTreePath();
    if (path.isEmpty() || !QApplication::clipboard()) {
        return;
    }

    QApplication::clipboard()->setText(ProjectFileOperations::pathForClipboard(path));
    setStatus(QString::fromUtf8("تم نسخ المسار"));
}

void MainWindow::openProjectTreeContainingFolder()
{
    const QString path = activeProjectTreePath();
    if (path.isEmpty()) {
        return;
    }

    const QString folderPath = ProjectFileOperations::containingFolder(path);
    if (folderPath.isEmpty()) {
        return;
    }

    QProcess::startDetached(QStringLiteral("explorer.exe"), {QDir::toNativeSeparators(folderPath)});
}

void MainWindow::refreshProjectTree()
{
    if (projectRoot.isEmpty() || !fileSystemModel || !projectTree) {
        return;
    }

    fileSystemModel->setRootPath(projectRoot);
    const QModelIndex rootIndex = fileSystemModel->index(projectRoot);
    projectTree->setRootIndex(rootIndex);
    projectTree->expand(rootIndex);
    setStatus(QString::fromUtf8("تم تحديث المشروع"));
}

QModelIndex MainWindow::activeProjectTreeIndex() const
{
    if (projectTreeContextIndex.isValid()) {
        return projectTreeContextIndex.siblingAtColumn(0);
    }
    if (projectTree && projectTree->currentIndex().isValid()) {
        return projectTree->currentIndex().siblingAtColumn(0);
    }
    return QModelIndex();
}

QString MainWindow::activeProjectTreePath() const
{
    const QModelIndex index = activeProjectTreeIndex();
    return index.isValid() && fileSystemModel ? fileSystemModel->filePath(index) : QString();
}

QString MainWindow::activeProjectTreeFolderPath() const
{
    const QString path = activeProjectTreePath();
    if (path.isEmpty()) {
        return projectRoot;
    }

    const QFileInfo info(path);
    return info.isDir() ? info.absoluteFilePath() : info.absolutePath();
}

void MainWindow::clearEditorsForDeletedPath(const QString &path)
{
    for (int i = editorTabs ? editorTabs->count() - 1 : -1; i >= 0; --i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface) {
            continue;
        }

        const QString openPath = QFileInfo(surface->currentFilePath()).absoluteFilePath();
        if (openPath.isEmpty() || !ProjectModel::pathIsSameOrInside(openPath, path)) {
            continue;
        }

        if (editorTabs->count() == 1) {
            surface->resetForNewFile();
            updateEditorTabTitle(surface);
            setCurrentEditor(surface);
            continue;
        }

        editorTabs->removeTab(i);
        surface->deleteLater();
    }

    setCurrentEditor(qobject_cast<EditorSurface *>(editorTabs->currentWidget()));
    refreshEditorProblems();
}

void MainWindow::openSettings()
{
    const QStringList orderedFamilies = arabicEditorFontFamilies();
    const RuntimeDiagnostics diagnostics = runtime.diagnostics(1500);
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

    auto *projectsPage = new QWidget(pages);
    projectsPage->setObjectName(QStringLiteral("recentProjectsPage"));
    projectsPage->setLayoutDirection(Qt::RightToLeft);
    auto *projectsLayout = new QVBoxLayout(projectsPage);
    auto *recentProjects = new QListWidget(projectsPage);
    recentProjects->setObjectName(QStringLiteral("recentProjectsList"));
    recentProjects->setLayoutDirection(Qt::RightToLeft);
    recentProjects->addItems(settingsState.recentProjects);
    projectsLayout->addWidget(recentProjects);

    pages->addTab(editorPage, QString::fromUtf8("المحرر"));
    pages->addTab(runtimePage, QString::fromUtf8("التشغيل"));
    pages->addTab(projectsPage, QString::fromUtf8("المشاريع"));
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
    for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
        if (auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i))) {
            applyEditorFont(surface);
        }
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
    fileSystemModel->setRootPath(projectRoot);
    projectTree->setRootIndex(fileSystemModel->index(projectRoot));
    applyWorkspaceSettingsToOpenEditors();
    settings.addRecentProject(projectRoot);
    setStatus(QString::fromUtf8("المشروع: %1").arg(projectRoot));
    return true;
}

bool MainWindow::openEditorFile(const QString &path)
{
    const QString normalizedPath = QFileInfo(path).absoluteFilePath();
    const int existingIndex = workbenchState.findEditorSessionByPath(normalizedPath);
    if (existingIndex >= 0 && editorTabs && existingIndex < editorTabs->count()) {
        editorTabs->setCurrentIndex(existingIndex);
        return true;
    }

    if (editor && editor->isDirty() && !confirmUnsavedDocuments(UnsavedChangesOperation::OpenFile)) {
        return false;
    }

    EditorSurface *target = editor;
    if (!target || !target->currentFilePath().isEmpty() || target->isDirty() || editorTabs->count() > 1) {
        target = createEditorTab(QFileInfo(normalizedPath).fileName());
    }

    QString error;
    if (!target->openFile(normalizedPath, &error)) {
        QMessageBox::warning(this, QString::fromUtf8("تعذر فتح الملف"), error);
        return false;
    }
    setCurrentEditor(target);
    syncEditorSession(target);
    updateEditorTabTitle(target);
    if (projectRoot.isEmpty()) {
        loadProject(QFileInfo(normalizedPath).absolutePath());
    }
    settings.addRecentFile(normalizedPath);
    setStatus(QString::fromUtf8("فتح: %1").arg(QFileInfo(normalizedPath).fileName()));
    return true;
}

EditorSurface *MainWindow::createEditorTab(const QString &title)
{
    auto *surface = new EditorSurface(editorTabs);
    applyEditorFont(surface);
    applyWorkspaceSettings(surface);
    const int index = editorTabs->addTab(surface, title);
    workbenchState.addEditorSession(surface->currentFilePath());
    workbenchState.setCurrentEditorSessionIndex(index);
    editorTabs->setCurrentIndex(index);
    setCurrentEditor(surface);

    connect(surface, &EditorSurface::filePathChanged, this, [this, surface](const QString &) {
        syncEditorSession(surface);
        updateEditorTabTitle(surface);
        updateBreadcrumbBar();
        updateStatusIndicators();
        if (surface == editor) {
            setWindowTitle(surface->currentFilePath().isEmpty()
                ? QString::fromUtf8("استوديو لسان")
                : QString::fromUtf8("استوديو لسان - %1").arg(QFileInfo(surface->currentFilePath()).fileName()));
        }
    });
    connect(surface, &EditorSurface::dirtyStateChanged, this, [this, surface](bool) {
        syncEditorSession(surface);
        updateEditorTabTitle(surface);
        updateBreadcrumbBar();
        updateStatusIndicators();
    });
    connect(surface->document(), &QTextDocument::contentsChanged, this, [this, surface]() {
        if (surface == editor) {
            refreshEditorProblems();
            updateStatusIndicators();
        }
    });

    updateEditorTabTitle(surface);
    return surface;
}

void MainWindow::applyEditorFont(EditorSurface *surface)
{
    if (!surface) {
        return;
    }

    const QString family = resolvedArabicEditorFontFamily(settings.editorFontFamily());
    QFont configuredFont = surface->font();
    configuredFont.setFamily(family);
    configuredFont.setPointSize(settings.editorFontSize());
    configuredFont.setStyleHint(QFont::Monospace);
    surface->setFont(configuredFont);

    QString stylesheetFamily = family;
    stylesheetFamily.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    stylesheetFamily.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    surface->setStyleSheet(QStringLiteral("font-family: \"%1\"; font-size: %2pt;")
        .arg(stylesheetFamily)
        .arg(settings.editorFontSize()));
    surface->setTabStopDistance(surface->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
}

void MainWindow::applyWorkspaceSettings(EditorSurface *surface)
{
    if (!surface) {
        return;
    }

    surface->setTrimTrailingWhitespaceOnSave(workspaceSettings.trimTrailingWhitespaceOnSave);
}

void MainWindow::applyWorkspaceSettingsToOpenEditors()
{
    if (!editorTabs) {
        return;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        applyWorkspaceSettings(qobject_cast<EditorSurface *>(editorTabs->widget(i)));
    }
}

void MainWindow::applyThemePreference()
{
    const QString preference = settings.themePreference();
    setProperty("themePreference", preference);
    setStyleSheet(preference == QStringLiteral("light") ? lightWorkbenchStyleSheet() : darkThemeStyleSheet);
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

void MainWindow::syncEditorSession(EditorSurface *surface)
{
    if (!surface || !editorTabs) {
        return;
    }

    const int index = editorTabs->indexOf(surface);
    if (index < 0) {
        return;
    }

    workbenchState.setEditorSessionPath(index, surface->currentFilePath());
    workbenchState.setEditorSessionDirty(index, surface->isDirty());
}

void MainWindow::setCurrentEditor(EditorSurface *surface)
{
    if (!surface) {
        return;
    }

    const int currentIndex = editorTabs ? editorTabs->indexOf(surface) : -1;
    workbenchState.setCurrentEditorSessionIndex(currentIndex);
    syncEditorSession(surface);

    for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
        if (auto *tabEditor = qobject_cast<EditorSurface *>(editorTabs->widget(i))) {
            tabEditor->setObjectName(tabEditor == surface ? QStringLiteral("editorSurface") : QStringLiteral("editorSurfaceInactive"));
        }
    }

    editor = surface;
    setWindowTitle(editor->currentFilePath().isEmpty()
        ? QString::fromUtf8("استوديو لسان")
        : QString::fromUtf8("استوديو لسان - %1").arg(QFileInfo(editor->currentFilePath()).fileName()));
    updateBreadcrumbBar();
    updateStatusIndicators();
    refreshEditorProblems();
}

void MainWindow::updateEditorTabTitle(EditorSurface *surface)
{
    if (!surface || !editorTabs) {
        return;
    }

    const int index = editorTabs->indexOf(surface);
    if (index < 0) {
        return;
    }

    syncEditorSession(surface);
    const auto sessions = workbenchState.editorSessions();
    const EditorSessionState session = index < sessions.size() ? sessions.at(index) : EditorSessionState {};

    QString title = session.path.isEmpty()
        ? QString::fromUtf8("ملف جديد")
        : QFileInfo(session.path).fileName();
    if (session.dirty) {
        title.prepend(QLatin1Char('*'));
    }
    editorTabs->setTabText(index, title);
}

void MainWindow::closeEditorTab(int index)
{
    auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(index));
    if (!surface) {
        return;
    }

    EditorSurface *previousEditor = editor;
    setCurrentEditor(surface);
    if (!confirmSaveIfDirty()) {
        setCurrentEditor(previousEditor);
        return;
    }

    if (editorTabs->count() == 1) {
        surface->resetForNewFile();
        syncEditorSession(surface);
        updateEditorTabTitle(surface);
        refreshEditorProblems();
        return;
    }

    editorTabs->removeTab(index);
    workbenchState.removeEditorSession(index);
    surface->deleteLater();
    setCurrentEditor(qobject_cast<EditorSurface *>(editorTabs->currentWidget()));
    refreshEditorProblems();
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
    if (bottomPanelTabs) {
        bottomPanelTabs->setCurrentWidget(outputPanel);
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showTerminalPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanelTabs) {
        bottomPanelTabs->setCurrentWidget(terminalPanel);
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showProblemsPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanelTabs) {
        bottomPanelTabs->setCurrentWidget(problemsPanel);
    }
    outputDock->raise();
    resizeDocks({outputDock}, {190}, Qt::Vertical);
}

void MainWindow::showSearchResultsPanel()
{
    if (!outputDock->isVisible()) {
        outputDock->show();
    }
    if (bottomPanelTabs) {
        bottomPanelTabs->setCurrentWidget(searchResultsPanel);
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

QVector<DocumentRecord> MainWindow::openDocumentRecords() const
{
    QVector<DocumentRecord> records;
    if (!editorTabs) {
        return records;
    }

    for (int i = 0; i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface) {
            continue;
        }

        DocumentRecord record;
        record.id = DocumentId(i + 1);
        record.path = surface->currentFilePath();
        record.text = surface->toPlainText();
        record.dirty = surface->isDirty();
        record.identity = record.path.isEmpty() ? DocumentFileIdentity {} : DocumentFileIO::identityForPath(record.path);
        record.lineEnding = DocumentFileIO::detectLineEnding(record.text);
        records.push_back(record);
    }
    return records;
}

bool MainWindow::confirmUnsavedDocuments(UnsavedChangesOperation operation)
{
    const UnsavedChangesRequest request = UnsavedChangesGuard::requestFor(openDocumentRecords(), operation);
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
        return true;
    }

    for (int i = 0; editorTabs && i < editorTabs->count(); ++i) {
        auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
        if (!surface || !surface->isDirty()) {
            continue;
        }

        setCurrentEditor(surface);
        saveFile();
        if (surface->isDirty()) {
            return false;
        }
    }
    return true;
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
    if (activeRuntimeProcess && activeRuntimeProcess->state() != QProcess::NotRunning) {
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
    const RuntimeLaunchPlan plan = runtime.buildLaunchPlan(action, title, runFilePath, workingDirectory, reloadAfterSuccess);
    startRuntimeLaunchPlan(plan, true);
}

void MainWindow::startRuntimeLaunchPlan(const RuntimeLaunchPlan &plan, bool recordHistory)
{
    if (recordHistory) {
        runtimeHistory.recordLaunch(plan);
    }

    activeRuntimeTitle = plan.title;
    activeRuntimeHandledError = false;
    activeRuntimeStdout.clear();
    activeRuntimeStderr.clear();
    showOutputPanel();
    outputTranscript.clear();
    const QString initialText = plan.initialOutput.section(QLatin1Char('\n'), 1).trimmed();
    outputTranscript.append(OutputTranscriptChannel::System, plan.title, initialText);
    renderOutputTranscript();
    setStatus(plan.runningStatus);
    setRuntimeActionsRunning(true);

    activeRuntimeProcess = new QProcess(this);
    activeRuntimeProcess->setProgram(plan.command.program);
    activeRuntimeProcess->setArguments(plan.command.arguments);
    activeRuntimeProcess->setWorkingDirectory(plan.command.workingDirectory);
    activeRuntimeProcess->setProcessEnvironment(runtime.processEnvironment());
    activeRuntimeProcess->setProperty("reloadAfterSuccess", plan.reloadAfterSuccess);
    activeRuntimeProcess->setProperty("runFilePath", plan.filePath);
    connect(activeRuntimeProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::appendRuntimeStdout);
    connect(activeRuntimeProcess, &QProcess::readyReadStandardError, this, &MainWindow::appendRuntimeStderr);
    connect(activeRuntimeProcess, &QProcess::finished, this, &MainWindow::finishRuntimeProcess);
    connect(activeRuntimeProcess, &QProcess::errorOccurred, this, &MainWindow::handleRuntimeProcessError);

    activeRuntimeTimer.start();
    runtimeTimeoutTimer->start();
    activeRuntimeProcess->start();
}

void MainWindow::appendRuntimeOutput(const QString &label, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    showOutputPanel();
    OutputTranscriptChannel channel = OutputTranscriptChannel::System;
    if (label == QStringLiteral("stdout")) {
        channel = OutputTranscriptChannel::Stdout;
    } else if (label == QStringLiteral("stderr")) {
        channel = OutputTranscriptChannel::Stderr;
    }
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

void MainWindow::completeRuntimeProcess(const QString &statusText)
{
    if (!activeRuntimeProcess) {
        return;
    }

    const bool reloadAfterSuccess = activeRuntimeProcess->property("reloadAfterSuccess").toBool();
    const QString runFilePath = activeRuntimeProcess->property("runFilePath").toString();
    const int exitCode = activeRuntimeProcess->exitCode();
    QProcess *finishedProcess = activeRuntimeProcess;
    activeRuntimeProcess = nullptr;
    if (runtimeTimeoutTimer) {
        runtimeTimeoutTimer->stop();
    }
    setRuntimeActionsRunning(false);
    setStatus(statusText);
    finishedProcess->deleteLater();

    if (reloadAfterSuccess && exitCode == 0) {
        QString error;
        editor->openFile(runFilePath, &error);
    }
}

void MainWindow::setRuntimeActionsRunning(bool running)
{
    if (runAction) {
        runAction->setEnabled(!running);
    }
    if (lintAction) {
        lintAction->setEnabled(!running);
    }
    if (formatAction) {
        formatAction->setEnabled(!running);
    }
    if (cancelRunAction) {
        cancelRunAction->setEnabled(running);
    }
}

void MainWindow::restoreWorkbenchSession()
{
    const SavedWorkbenchSession session = settings.savedWorkbenchSession();
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

    if (openedAnyFile && editorTabs) {
        const int activeIndex = qBound(0, session.activeFileIndex, editorTabs->count() - 1);
        editorTabs->setCurrentIndex(activeIndex);
    }

    if (bottomPanelTabs) {
        if (auto *panel = bottomPanelForId(session.bottomPanelId)) {
            bottomPanelTabs->setCurrentWidget(panel);
        }
    }
}

void MainWindow::saveWorkbenchSession()
{
    SavedWorkbenchSession session;
    session.projectRoot = projectRoot;
    if (editorTabs) {
        for (int i = 0; i < editorTabs->count(); ++i) {
            auto *surface = qobject_cast<EditorSurface *>(editorTabs->widget(i));
            if (!surface || surface->currentFilePath().isEmpty()) {
                continue;
            }
            session.openFiles.append(surface->currentFilePath());
            if (i == editorTabs->currentIndex()) {
                session.activeFileIndex = session.openFiles.size() - 1;
            }
        }
    }
    if (session.activeFileIndex < 0 && !session.openFiles.isEmpty()) {
        session.activeFileIndex = 0;
    }
    session.bottomPanelId = bottomPanelTabs ? bottomPanelId(bottomPanelTabs->currentWidget()) : QString();
    settings.saveWorkbenchSession(session);
}

QString MainWindow::bottomPanelId(QWidget *panel) const
{
    if (panel == terminalPanel) {
        return QStringLiteral("terminal");
    }
    if (panel == outputPanel) {
        return QStringLiteral("output");
    }
    if (panel == problemsPanel) {
        return QStringLiteral("problems");
    }
    if (panel == searchResultsPanel) {
        return QStringLiteral("search");
    }
    if (panel == debugPanel) {
        return QStringLiteral("debug");
    }
    return QString();
}

QWidget *MainWindow::bottomPanelForId(const QString &id) const
{
    if (id == QStringLiteral("terminal")) {
        return terminalPanel;
    }
    if (id == QStringLiteral("output")) {
        return outputPanel;
    }
    if (id == QStringLiteral("problems")) {
        return problemsPanel;
    }
    if (id == QStringLiteral("search")) {
        return searchResultsPanel;
    }
    if (id == QStringLiteral("debug")) {
        return debugPanel;
    }
    return nullptr;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsavedDocuments(UnsavedChangesOperation::Exit)) {
        event->ignore();
        return;
    }

    saveWorkbenchSession();
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
