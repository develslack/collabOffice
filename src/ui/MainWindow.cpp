#include "MainWindow.h"
#include "IWorkspaceModule.h"
#include "LanguageManager.h"
#include "DatabaseManager.h"

#include <QTabWidget>
#include <QDockWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QStatusBar>
#include <QEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QStyle>
#include <utility>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_layoutMode(UiLayoutMode::Ribbon)
    , m_connectionState(ConnectionState::MariaDbOnline)
    , m_pendingSyncCount(0)
    , m_unreadNotifications(0)
    , m_topHeaderBar(nullptr)
    , m_btnNewDoc(nullptr)
    , m_btnNewSheet(nullptr)
    , m_btnOpenBrowser(nullptr)
    , m_btnAdminPanel(nullptr)
    , m_btnNotifications(nullptr)
    , m_btnConnectionBadge(nullptr)
    , m_comboUiMode(nullptr)
    , m_comboLanguage(nullptr)
    , m_btnLogout(nullptr)
    , m_commandHostWidget(nullptr)
    , m_ribbonTabWidget(nullptr)
    , m_classicMenuBar(nullptr)
    , m_classicToolBar(nullptr)
    , m_workspaceTabs(nullptr)
    , m_contextualDock(nullptr)
    , m_emptyDockPlaceholder(nullptr)
    , m_lblModuleStatus(nullptr)
    , m_lblShortcutProfile(nullptr)
{
    setupShellUi();
    applyCorporateStyleSheet();

    QSettings settings(QStringLiteral("CollabOffice"), QStringLiteral("ClientPreferences"));
    const QString savedMode = settings.value(QStringLiteral("ui/layoutMode"), QStringLiteral("RIBBON")).toString();
    m_layoutMode = (savedMode == QStringLiteral("CLASSIC")) ? UiLayoutMode::Classic : UiLayoutMode::Ribbon;

    m_comboUiMode->blockSignals(true);
    m_comboUiMode->setCurrentIndex(m_layoutMode == UiLayoutMode::Classic ? 1 : 0);
    m_comboUiMode->blockSignals(false);

    retranslateUi();
    setConnectionState(ConnectionState::SqliteOfflineFallback, 0);
    setUnreadNotificationsCount(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupShellUi()
{
    setMinimumSize(960, 620);
    resize(1280, 800);

    setupTopHeaderBar();
    setupCentralWorkspace();
    setupContextualDock();
    setupStatusBar();
}

void MainWindow::setupTopHeaderBar()
{
    m_topHeaderBar = new QToolBar(this);
    m_topHeaderBar->setObjectName(QStringLiteral("TopShellHeaderBar"));
    m_topHeaderBar->setMovable(false);
    m_topHeaderBar->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, m_topHeaderBar);

    m_btnNewDoc = new QPushButton(this);
    m_btnNewSheet = new QPushButton(this);
    m_btnOpenBrowser = new QPushButton(this);
    m_btnAdminPanel = new QPushButton(this);

    m_topHeaderBar->addWidget(m_btnNewDoc);
    m_topHeaderBar->addWidget(m_btnNewSheet);
    m_topHeaderBar->addWidget(m_btnOpenBrowser);
    m_topHeaderBar->addSeparator();
    m_topHeaderBar->addWidget(m_btnAdminPanel);

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_topHeaderBar->addWidget(spacer);

    m_btnConnectionBadge = new QPushButton(this);
    m_btnConnectionBadge->setObjectName(QStringLiteral("ConnectionBadge"));
    m_topHeaderBar->addWidget(m_btnConnectionBadge);

    m_btnNotifications = new QPushButton(this);
    m_btnNotifications->setObjectName(QStringLiteral("NotificationsButton"));
    m_topHeaderBar->addWidget(m_btnNotifications);

    m_comboUiMode = new QComboBox(this);
    m_comboUiMode->addItem(QString(), static_cast<int>(UiLayoutMode::Ribbon));
    m_comboUiMode->addItem(QString(), static_cast<int>(UiLayoutMode::Classic));
    m_topHeaderBar->addWidget(m_comboUiMode);

    m_comboLanguage = new QComboBox(this);
    const LanguageManager &langMgr = LanguageManager::instance();
    for (const QString &code : langMgr.supportedLanguages()) {
        m_comboLanguage->addItem(langMgr.languageDisplayName(code), code);
    }
    const int currentLangIdx = m_comboLanguage->findData(langMgr.currentLanguage());
    if (currentLangIdx >= 0) {
        m_comboLanguage->setCurrentIndex(currentLangIdx);
    }
    m_topHeaderBar->addWidget(m_comboLanguage);

    m_topHeaderBar->addSeparator();
    m_btnLogout = new QPushButton(this);
    m_btnLogout->setObjectName(QStringLiteral("LogoutButton"));
    m_topHeaderBar->addWidget(m_btnLogout);

    connect(m_btnNewDoc, &QPushButton::clicked, this, &MainWindow::newTextDocumentRequested);
    connect(m_btnNewSheet, &QPushButton::clicked, this, &MainWindow::newSpreadsheetRequested);
    connect(m_btnOpenBrowser, &QPushButton::clicked, this, &MainWindow::openDocumentBrowserRequested);
    connect(m_btnAdminPanel, &QPushButton::clicked, this, &MainWindow::openUserAdminRequested);
    connect(m_btnNotifications, &QPushButton::clicked, this, &MainWindow::openNotificationsRequested);
    connect(m_btnConnectionBadge, &QPushButton::clicked, this, &MainWindow::syncNowRequested);
    connect(m_btnLogout, &QPushButton::clicked, this, &MainWindow::onLogoutButtonClicked);
    connect(m_comboUiMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onUiModeComboChanged);
    connect(m_comboLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageComboChanged);
}

void MainWindow::setupCentralWorkspace()
{
    QWidget *centralContainer = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_commandHostWidget = new QWidget(centralContainer);
    m_commandHostWidget->setObjectName(QStringLiteral("CommandHostWidget"));
    QVBoxLayout *cmdLayout = new QVBoxLayout(m_commandHostWidget);
    cmdLayout->setContentsMargins(4, 2, 4, 2);
    cmdLayout->setSpacing(2);

    m_ribbonTabWidget = new QTabWidget(m_commandHostWidget);
    m_ribbonTabWidget->setObjectName(QStringLiteral("RibbonTabWidget"));
    m_ribbonTabWidget->setMaximumHeight(104);

    m_classicMenuBar = new QMenuBar(m_commandHostWidget);
    m_classicToolBar = new QToolBar(m_commandHostWidget);
    m_classicToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    cmdLayout->addWidget(m_ribbonTabWidget);
    cmdLayout->addWidget(m_classicMenuBar);
    cmdLayout->addWidget(m_classicToolBar);

    mainLayout->addWidget(m_commandHostWidget);

    m_workspaceTabs = new QTabWidget(centralContainer);
    m_workspaceTabs->setDocumentMode(true);
    m_workspaceTabs->setTabsClosable(true);
    m_workspaceTabs->setMovable(true);
    mainLayout->addWidget(m_workspaceTabs, 1);

    setCentralWidget(centralContainer);

    connect(m_workspaceTabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_workspaceTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
}

void MainWindow::setupContextualDock()
{
    m_contextualDock = new QDockWidget(this);
    m_contextualDock->setObjectName(QStringLiteral("ContextualInspectorDock"));
    m_contextualDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_contextualDock->setMinimumWidth(270);

    m_emptyDockPlaceholder = new QLabel(m_contextualDock);
    m_emptyDockPlaceholder->setAlignment(Qt::AlignCenter);
    m_emptyDockPlaceholder->setWordWrap(true);
    m_contextualDock->setWidget(m_emptyDockPlaceholder);

    addDockWidget(Qt::RightDockWidgetArea, m_contextualDock);
}

void MainWindow::setupStatusBar()
{
    m_lblModuleStatus = new QLabel(this);
    m_lblShortcutProfile = new QLabel(this);

    statusBar()->addWidget(m_lblModuleStatus, 1);
    statusBar()->addPermanentWidget(m_lblShortcutProfile);
}

void MainWindow::registerModule(IWorkspaceModule *module, bool activateImmediately)
{
    if (!module) {
        return;
    }

    if (m_modules.contains(module->moduleId())) {
        for (int i = 0; i < m_workspaceTabs->count(); ++i) {
            QWidget *w = m_workspaceTabs->widget(i);
            if (w && w->property("moduleId").toString() == module->moduleId()) {
                m_workspaceTabs->setCurrentIndex(i);
                break;
            }
        }
        return;
    }

    m_modules.insert(module->moduleId(), module);

    const QString langCode = LanguageManager::instance().currentLanguage();
    module->applyShortcutProfile(langCode == QStringLiteral("es") ? QStringLiteral("ES") : QStringLiteral("EN"));

    QWidget *widget = module->centralWidget();
    widget->setProperty("moduleId", module->moduleId());

    const int prevIndex = m_workspaceTabs->currentIndex();
    const int tabIdx = m_workspaceTabs->addTab(widget, module->moduleIcon(), module->displayTitle());

    if (activateImmediately) {
        if (prevIndex != tabIdx) {
            // Triggers currentChanged -> onTabChanged(), rebuilding Ribbon, Dock and StatusBar once
            m_workspaceTabs->setCurrentIndex(tabIdx);
        } else {
            rebuildCommandAreaForModule(module);
            updateContextualDockForModule(module);
            m_lblModuleStatus->setText(module->statusSummary());
        }
    }
}

void MainWindow::unregisterModule(const QString &moduleId)
{
    IWorkspaceModule *module = m_modules.take(moduleId);
    if (!module) {
        return;
    }

    for (int i = 0; i < m_workspaceTabs->count(); ++i) {
        QWidget *w = m_workspaceTabs->widget(i);
        if (w && w->property("moduleId").toString() == moduleId) {
            m_workspaceTabs->removeTab(i);
            break;
        }
    }

    if (m_modules.isEmpty()) {
        rebuildCommandAreaForModule(nullptr);
        updateContextualDockForModule(nullptr);
        m_lblModuleStatus->clear();
    }
}

void MainWindow::closeAllModules()
{
    const QStringList ids = m_modules.keys();
    for (const QString &id : ids) {
        unregisterModule(id);
    }
}

IWorkspaceModule *MainWindow::activeModule() const
{
    QWidget *currentWidget = m_workspaceTabs->currentWidget();
    if (!currentWidget) {
        return nullptr;
    }
    const QString id = currentWidget->property("moduleId").toString();
    return m_modules.value(id, nullptr);
}

void MainWindow::setCurrentUserSession(const UserSession &session, const QString &hostLabel)
{
    m_currentSession = session;
    m_hostLabel = hostLabel;

    m_btnAdminPanel->setVisible(session.isAdmin());
    retranslateUi();
}

void MainWindow::setUiLayoutMode(UiLayoutMode mode)
{
    if (m_layoutMode == mode) {
        return;
    }

    m_layoutMode = mode;

    QSettings settings(QStringLiteral("CollabOffice"), QStringLiteral("ClientPreferences"));
    settings.setValue(QStringLiteral("ui/layoutMode"),
                      m_layoutMode == UiLayoutMode::Classic ? QStringLiteral("CLASSIC") : QStringLiteral("RIBBON"));

    rebuildCommandAreaForModule(activeModule());
    emit uiLayoutModeChanged(m_layoutMode);
}

MainWindow::UiLayoutMode MainWindow::uiLayoutMode() const
{
    return m_layoutMode;
}

void MainWindow::setConnectionState(ConnectionState state, int pendingSyncCount)
{
    m_connectionState = state;
    m_pendingSyncCount = pendingSyncCount;

    switch (m_connectionState) {
    case ConnectionState::MariaDbOnline:
        m_btnConnectionBadge->setText(tr("Server: MariaDB Online"));
        m_btnConnectionBadge->setProperty("statusLevel", QStringLiteral("online"));
        break;
    case ConnectionState::SqliteOfflineFallback:
        m_btnConnectionBadge->setText(tr("Offline Mode (Local SQLite)"));
        m_btnConnectionBadge->setProperty("statusLevel", QStringLiteral("offline"));
        break;
    case ConnectionState::SyncPending:
        m_btnConnectionBadge->setText(tr("Sync Pending (%1)").arg(m_pendingSyncCount));
        m_btnConnectionBadge->setProperty("statusLevel", QStringLiteral("pending"));
        break;
    }

    m_btnConnectionBadge->style()->unpolish(m_btnConnectionBadge);
    m_btnConnectionBadge->style()->polish(m_btnConnectionBadge);
}

void MainWindow::setUnreadNotificationsCount(int count)
{
    m_unreadNotifications = count;
    m_btnNotifications->setText(tr("Proposals (%1)").arg(m_unreadNotifications));
}

bool MainWindow::promptSaveForModule(IWorkspaceModule *module)
{
    if (!module || !module->hasUnsavedChanges()) {
        return true;
    }

    // Activate tab so the user sees which document has unsaved changes
    for (int i = 0; i < m_workspaceTabs->count(); ++i) {
        QWidget *w = m_workspaceTabs->widget(i);
        if (w && w->property("moduleId").toString() == module->moduleId()) {
            m_workspaceTabs->setCurrentIndex(i);
            break;
        }
    }

    const QMessageBox::StandardButton choice = QMessageBox::question(
        this,
        tr("Unsaved Changes"),
        tr("The document '%1' has unsaved changes.\nDo you want to save your changes before closing?")
            .arg(module->displayTitle()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (choice == QMessageBox::Cancel) {
        return false;
    }
    if (choice == QMessageBox::Save) {
        return module->saveChanges();
    }
    return true; // Discard
}

bool MainWindow::confirmAndSaveAllOpenModules()
{
    for (int i = 0; i < m_workspaceTabs->count(); ++i) {
        QWidget *w = m_workspaceTabs->widget(i);
        if (!w) {
            continue;
        }
        IWorkspaceModule *mod = m_modules.value(w->property("moduleId").toString(), nullptr);
        if (mod && !promptSaveForModule(mod)) {
            return false;
        }
    }
    return true;
}

void MainWindow::onLogoutButtonClicked()
{
    if (!confirmAndSaveAllOpenModules()) {
        return;
    }
    closeAllModules();
    emit logoutRequested();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmAndSaveAllOpenModules()) {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    IWorkspaceModule *current = activeModule();
    rebuildCommandAreaForModule(current);
    updateContextualDockForModule(current);
    m_lblModuleStatus->setText(current ? current->statusSummary() : QString());
}

void MainWindow::onTabCloseRequested(int index)
{
    QWidget *w = m_workspaceTabs->widget(index);
    if (!w) {
        return;
    }
    const QString moduleId = w->property("moduleId").toString();
    IWorkspaceModule *mod = m_modules.value(moduleId, nullptr);
    if (mod && !promptSaveForModule(mod)) {
        return;
    }
    unregisterModule(moduleId);
}

void MainWindow::onLanguageComboChanged(int index)
{
    const QString langCode = m_comboLanguage->itemData(index).toString();
    LanguageManager::instance().setLanguage(langCode);
}

void MainWindow::onUiModeComboChanged(int index)
{
    const int modeVal = m_comboUiMode->itemData(index).toInt();
    setUiLayoutMode(static_cast<UiLayoutMode>(modeVal));
}

void MainWindow::rebuildCommandAreaForModule(IWorkspaceModule *module)
{
    const bool isRibbon = (m_layoutMode == UiLayoutMode::Ribbon);
    m_ribbonTabWidget->setVisible(isRibbon);
    m_classicMenuBar->setVisible(!isRibbon);
    m_classicToolBar->setVisible(!isRibbon);

    if (isRibbon) {
        buildRibbonForModule(module);
    } else {
        buildClassicBarsForModule(module);
    }
}

void MainWindow::buildRibbonForModule(IWorkspaceModule *module)
{
    // Detach persistent module widgets so deleting temporary Ribbon pages never frees them
    for (QWidget *customW : std::as_const(m_activeRibbonCustomWidgets)) {
        if (customW) {
            customW->hide();
            customW->setParent(nullptr);
        }
    }
    m_activeRibbonCustomWidgets.clear();

    while (m_ribbonTabWidget->count() > 0) {
        QWidget *page = m_ribbonTabWidget->widget(0);
        m_ribbonTabWidget->removeTab(0);
        delete page;
    }

    if (!module) {
        return;
    }

    QMap<QString, QHBoxLayout *> tabLayouts;
    const QList<CommandGroupSpec> groups = module->commandGroups();

    for (const CommandGroupSpec &group : groups) {
        QHBoxLayout *rowLayout = tabLayouts.value(group.tabTitle, nullptr);
        if (!rowLayout) {
            QWidget *tabPage = new QWidget(m_ribbonTabWidget);
            rowLayout = new QHBoxLayout(tabPage);
            rowLayout->setContentsMargins(6, 3, 6, 3);
            rowLayout->setSpacing(6);
            rowLayout->addStretch(1);
            m_ribbonTabWidget->addTab(tabPage, group.tabTitle);
            tabLayouts.insert(group.tabTitle, rowLayout);
        }

        QFrame *groupBox = new QFrame();
        groupBox->setObjectName(QStringLiteral("RibbonGroupFrame"));
        QVBoxLayout *groupVBox = new QVBoxLayout(groupBox);
        groupVBox->setContentsMargins(6, 2, 6, 1);
        groupVBox->setSpacing(2);

        if (group.customWidget) {
            m_activeRibbonCustomWidgets.append(group.customWidget);
            group.customWidget->setParent(groupBox);
            group.customWidget->show();
            groupVBox->addWidget(group.customWidget, 1);
        } else {
            QHBoxLayout *buttonsRow = new QHBoxLayout();
            buttonsRow->setSpacing(3);
            for (QAction *act : group.actions) {
                if (!act) {
                    continue;
                }
                QToolButton *btn = new QToolButton(groupBox);
                btn->setDefaultAction(act);
                btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                btn->setAutoRaise(true);
                buttonsRow->addWidget(btn);
            }
            groupVBox->addLayout(buttonsRow, 1);
        }

        if (!group.groupTitle.isEmpty()) {
            QLabel *lblGroup = new QLabel(group.groupTitle, groupBox);
            lblGroup->setAlignment(Qt::AlignCenter);
            lblGroup->setObjectName(QStringLiteral("RibbonGroupCaption"));
            groupVBox->addWidget(lblGroup);
        }

        rowLayout->insertWidget(rowLayout->count() - 1, groupBox);
    }
}

void MainWindow::buildClassicBarsForModule(IWorkspaceModule *module)
{
    m_classicMenuBar->clear();
    m_classicToolBar->clear();

    if (!module) {
        return;
    }

    QMap<QString, QMenu *> menusByTitle;
    const QList<CommandGroupSpec> groups = module->commandGroups();

    for (const CommandGroupSpec &group : groups) {
        QMenu *menu = menusByTitle.value(group.tabTitle, nullptr);
        if (!menu) {
            menu = m_classicMenuBar->addMenu(group.tabTitle);
            menusByTitle.insert(group.tabTitle, menu);
        } else {
            menu->addSeparator();
        }

        for (QAction *act : group.actions) {
            if (!act) {
                continue;
            }
            menu->addAction(act);
            m_classicToolBar->addAction(act);
        }
        m_classicToolBar->addSeparator();
    }
}

void MainWindow::updateContextualDockForModule(IWorkspaceModule *module)
{
    if (module && module->contextualDockWidget()) {
        m_contextualDock->setWidget(module->contextualDockWidget());
        m_contextualDock->setWindowTitle(tr("Inspector - %1").arg(module->displayTitle()));
    } else {
        m_contextualDock->setWidget(m_emptyDockPlaceholder);
        m_contextualDock->setWindowTitle(tr("Inspector"));
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
        const QString langCode = LanguageManager::instance().currentLanguage();
        const QString shortcutProf = (langCode == QStringLiteral("es")) ? QStringLiteral("ES") : QStringLiteral("EN");

        for (IWorkspaceModule *module : std::as_const(m_modules)) {
            if (module) {
                module->retranslateUi();
                module->applyShortcutProfile(shortcutProf);
            }
        }
        rebuildCommandAreaForModule(activeModule());
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUi()
{
    if (m_currentSession.isValid()) {
        setWindowTitle(tr("CollabOffice - %1 (%2) [%3]")
                           .arg(m_currentSession.fullName, m_currentSession.systemRole, m_hostLabel));
    } else {
        setWindowTitle(tr("CollabOffice - Collaborative Office Suite"));
    }

    m_btnNewDoc->setText(tr("New Document"));
    m_btnNewSheet->setText(tr("New Spreadsheet"));
    m_btnOpenBrowser->setText(tr("Documents"));
    m_btnAdminPanel->setText(tr("Users & Groups"));
    m_btnLogout->setText(tr("Sign Out"));

    m_comboUiMode->setItemText(0, tr("UI: Ribbon (MS Office)"));
    m_comboUiMode->setItemText(1, tr("UI: Classic (LibreOffice)"));

    m_emptyDockPlaceholder->setText(tr("Open or select a document to view its properties, permissions and revision history."));

    const QString langCode = LanguageManager::instance().currentLanguage();
    const QString shortcutHint = (langCode == QStringLiteral("es"))
                                     ? tr("Shortcuts: Spanish (Ctrl+N Bold)")
                                     : tr("Shortcuts: English (Ctrl+B Bold)");
    m_lblShortcutProfile->setText(shortcutHint);

    setConnectionState(m_connectionState, m_pendingSyncCount);
    setUnreadNotificationsCount(m_unreadNotifications);

    for (int i = 0; i < m_workspaceTabs->count(); ++i) {
        QWidget *w = m_workspaceTabs->widget(i);
        if (!w) {
            continue;
        }
        IWorkspaceModule *mod = m_modules.value(w->property("moduleId").toString(), nullptr);
        if (mod) {
            m_workspaceTabs->setTabText(i, mod->displayTitle());
        }
    }
}

void MainWindow::applyCorporateStyleSheet()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow {
            background-color: #1E1E1E;
            color: #E2E8F0;
        }
        QToolBar#TopShellHeaderBar {
            background-color: #18181B;
            border-bottom: 1px solid #2E2E32;
            spacing: 6px;
            padding: 4px 8px;
        }
        QToolBar#TopShellHeaderBar QPushButton,
        QToolBar#TopShellHeaderBar QComboBox {
            background-color: #27272A;
            color: #F4F4F5;
            border: 1px solid #3F3F46;
            border-radius: 4px;
            padding: 4px 9px;
            font-size: 12px;
        }
        QToolBar#TopShellHeaderBar QPushButton:hover,
        QToolBar#TopShellHeaderBar QComboBox:hover {
            background-color: #3F3F46;
            border-color: #60A5FA;
        }
        QPushButton#LogoutButton {
            background-color: #7F1D1D;
            border-color: #991B1B;
        }
        QPushButton#LogoutButton:hover {
            background-color: #B91C1C;
        }
        QPushButton#ConnectionBadge[statusLevel="online"] {
            background-color: #14532D;
            border-color: #22C55E;
        }
        QPushButton#ConnectionBadge[statusLevel="offline"] {
            background-color: #78350F;
            border-color: #F59E0B;
        }
        QPushButton#ConnectionBadge[statusLevel="pending"] {
            background-color: #1E3A8A;
            border-color: #60A5FA;
        }
        QWidget#CommandHostWidget {
            background-color: #262629;
            border-bottom: 1px solid #38383D;
        }
        QTabWidget#RibbonTabWidget::pane {
            border: none;
            background-color: #2B2B2F;
        }
        QTabWidget#RibbonTabWidget QTabBar::tab {
            background-color: transparent;
            color: #CBD5E1;
            padding: 5px 14px;
            margin-right: 2px;
            border-bottom: 2px solid transparent;
            font-weight: 500;
        }
        QTabWidget#RibbonTabWidget QTabBar::tab:hover {
            color: #FFFFFF;
            background-color: #333338;
        }
        QTabWidget#RibbonTabWidget QTabBar::tab:selected {
            color: #FFFFFF;
            border-bottom: 2px solid #3B82F6;
            font-weight: 600;
        }
        QFrame#RibbonGroupFrame {
            border-right: 1px solid #3F3F46;
            background-color: transparent;
        }
        QFrame#RibbonGroupFrame QToolButton {
            color: #E4E4E7;
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 3px;
            padding: 2px 4px;
        }
        QFrame#RibbonGroupFrame QToolButton:hover {
            background-color: #3F3F46;
            border-color: #52525B;
        }
        QFrame#RibbonGroupFrame QToolButton:checked {
            background-color: #1D4ED8;
            border-color: #60A5FA;
            color: #FFFFFF;
        }
        QLabel#RibbonGroupCaption {
            color: #9CA3AF;
            font-size: 10px;
        }
        QTabWidget::pane {
            border: none;
            background-color: #202023;
        }
        QTabBar::tab {
            background-color: #27272A;
            color: #A1A1AA;
            padding: 6px 14px;
            border-right: 1px solid #3F3F46;
        }
        QTabBar::tab:selected {
            background-color: #323236;
            color: #FFFFFF;
            font-weight: 600;
        }
        QDockWidget {
            background-color: #252528;
            color: #E4E4E7;
            titlebar-close-icon: none;
        }
        QDockWidget::title {
            background-color: #1F1F22;
            padding: 6px;
            border-bottom: 1px solid #38383D;
        }
        QStatusBar {
            background-color: #18181B;
            color: #A1A1AA;
            border-top: 1px solid #2E2E32;
        }
        QStatusBar QLabel {
            color: #CBD5E1;
            font-size: 11px;
        }
    )"));
}
