#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QList>
#include "DatabaseManager.h"

class QTabWidget;
class QDockWidget;
class QToolBar;
class QMenuBar;
class QComboBox;
class QPushButton;
class QLabel;
class QAction;
class QCloseEvent;
class IWorkspaceModule;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum class UiLayoutMode {
        Ribbon,
        Classic
    };
    Q_ENUM(UiLayoutMode)

    enum class ConnectionState {
        MariaDbOnline,
        SqliteOfflineFallback,
        SyncPending
    };
    Q_ENUM(ConnectionState)

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void registerModule(IWorkspaceModule *module, bool activateImmediately = true);
    void unregisterModule(const QString &moduleId);
    void closeAllModules();
    IWorkspaceModule *activeModule() const;

    void setCurrentUserSession(const UserSession &session, const QString &hostLabel);
    void setUiLayoutMode(UiLayoutMode mode);
    UiLayoutMode uiLayoutMode() const;

    void setConnectionState(ConnectionState state, int pendingSyncCount = 0);
    void setUnreadNotificationsCount(int count);

    // Checks every open module for unsaved changes and prompts the user before closing
    bool confirmAndSaveAllOpenModules();

signals:
    void newTextDocumentRequested();
    void newSpreadsheetRequested();
    void openDocumentBrowserRequested();
    void openNotificationsRequested();
    void openUserAdminRequested();
    void syncNowRequested();
    void logoutRequested();
    void uiLayoutModeChanged(MainWindow::UiLayoutMode mode);

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onLanguageComboChanged(int index);
    void onUiModeComboChanged(int index);
    void onLogoutButtonClicked();

private:
    void setupShellUi();
    void setupTopHeaderBar();
    void setupCentralWorkspace();
    void setupContextualDock();
    void setupStatusBar();
    void applyCorporateStyleSheet();

    bool promptSaveForModule(IWorkspaceModule *module);
    void rebuildCommandAreaForModule(IWorkspaceModule *module);
    void buildRibbonForModule(IWorkspaceModule *module);
    void buildClassicBarsForModule(IWorkspaceModule *module);
    void updateContextualDockForModule(IWorkspaceModule *module);
    void retranslateUi();

    UiLayoutMode m_layoutMode;
    ConnectionState m_connectionState;
    int m_pendingSyncCount;
    int m_unreadNotifications;
    UserSession m_currentSession;
    QString m_hostLabel;

    QToolBar *m_topHeaderBar;
    QPushButton *m_btnNewDoc;
    QPushButton *m_btnNewSheet;
    QPushButton *m_btnOpenBrowser;
    QPushButton *m_btnAdminPanel;
    QPushButton *m_btnNotifications;
    QPushButton *m_btnConnectionBadge;
    QComboBox *m_comboUiMode;
    QComboBox *m_comboLanguage;
    QPushButton *m_btnLogout;

    QWidget *m_commandHostWidget;
    QTabWidget *m_ribbonTabWidget;
    QList<QWidget *> m_activeRibbonCustomWidgets;
    QMenuBar *m_classicMenuBar;
    QToolBar *m_classicToolBar;

    QTabWidget *m_workspaceTabs;
    QDockWidget *m_contextualDock;
    QLabel *m_emptyDockPlaceholder;

    QLabel *m_lblModuleStatus;
    QLabel *m_lblShortcutProfile;

    QMap<QString, IWorkspaceModule *> m_modules;
};

#endif // MAINWINDOW_H
