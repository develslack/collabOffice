#ifndef IWORKSPACEMODULE_H
#define IWORKSPACEMODULE_H

#include <QString>
#include <QIcon>
#include <QList>
#include <QAction>
#include <QWidget>

struct CommandGroupSpec {
    QString tabTitle;          // Ribbon tab title or top-level Classic menu title
    QString groupTitle;        // Ribbon section group or Classic toolbar section
    QList<QAction *> actions;  // Actions owned by the module
};

class IWorkspaceModule
{
public:
    virtual ~IWorkspaceModule() = default;

    // Unique identifier for the module instance (e.g., "writer:<uuid>", "calc:<uuid>", "admin")
    virtual QString moduleId() const = 0;

    // Display title shown on the central tab
    virtual QString displayTitle() const = 0;

    // Icon shown on the central tab
    virtual QIcon moduleIcon() const = 0;

    // Main workspace widget mounted inside MainWindow's central tab area
    virtual QWidget *centralWidget() = 0;

    // Optional right-hand contextual panel (styles, cell inspector, comments, permissions)
    virtual QWidget *contextualDockWidget() { return nullptr; }

    // Declarative command groups rendered by MainWindow as Ribbon tabs or Classic menus/toolbars
    virtual QList<CommandGroupSpec> commandGroups() const = 0;

    // Optional status text displayed in MainWindow's status bar when this module is active
    virtual QString statusSummary() const { return QString(); }

    // Called by MainWindow whenever LanguageManager switches between lang_es and lang_en
    virtual void retranslateUi() = 0;

    // Called when user switches between Spanish (Ctrl+N) and English (Ctrl+B) shortcuts
    virtual void applyShortcutProfile(const QString &profileCode) { Q_UNUSED(profileCode); }

    // Unsaved changes lifecycle (queried by MainWindow on tab close, logout, or app exit)
    virtual bool hasUnsavedChanges() const { return false; }
    virtual bool saveChanges() { return true; }

};

#endif // IWORKSPACEMODULE_H
