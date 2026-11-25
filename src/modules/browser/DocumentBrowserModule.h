#ifndef DOCUMENTBROWSERMODULE_H
#define DOCUMENTBROWSERMODULE_H

#include <QObject>
#include "IWorkspaceModule.h"
#include "DatabaseManager.h"

class QTableWidget;
class QLineEdit;
class QPushButton;

class DocumentBrowserModule : public QObject, public IWorkspaceModule
{
    Q_OBJECT

public:
    explicit DocumentBrowserModule(const UserSession &session, QObject *parent = nullptr);
    ~DocumentBrowserModule() override = default;

    QString moduleId() const override;
    QString displayTitle() const override;
    QIcon moduleIcon() const override;
    QWidget *centralWidget() override;
    QList<CommandGroupSpec> commandGroups() const override;
    QString statusSummary() const override;
    void retranslateUi() override;

public slots:
    void refreshDocumentList();

signals:
    void openDocumentRequested(const DocumentSummary &docSummary);
    void createNewTextDocumentRequested();
    void createNewSpreadsheetRequested();

private slots:
    void onOpenSelectedClicked();
    void onShareSelectedClicked();
    void onSearchFilterChanged(const QString &text);

private:
    void buildUi();
    QString localizedAccessLabel(const QString &accessCode) const;

    UserSession m_session;
    QWidget *m_rootWidget;
    QLineEdit *m_editSearch;
    QTableWidget *m_tblDocuments;
    QPushButton *m_btnOpen;
    QPushButton *m_btnShare;

    QAction *m_actRefresh;
    QAction *m_actNewDoc;
    QAction *m_actNewSheet;
    QAction *m_actOpenDoc;
    QAction *m_actShareDoc;

    QList<DocumentSummary> m_cachedDocs;
};

#endif // DOCUMENTBROWSERMODULE_H
