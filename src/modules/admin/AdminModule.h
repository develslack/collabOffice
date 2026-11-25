#ifndef ADMINMODULE_H
#define ADMINMODULE_H

#include <QObject>
#include "IWorkspaceModule.h"
#include "DatabaseManager.h"

class QTabWidget;
class QTableWidget;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QPushButton;

class AdminModule : public QObject, public IWorkspaceModule
{
    Q_OBJECT

public:
    explicit AdminModule(const UserSession &adminSession, QObject *parent = nullptr);
    ~AdminModule() override = default;

    QString moduleId() const override;
    QString displayTitle() const override;
    QIcon moduleIcon() const override;
    QWidget *centralWidget() override;
    QList<CommandGroupSpec> commandGroups() const override;
    QString statusSummary() const override;
    void retranslateUi() override;

private slots:
    void onCreateUserClicked();
    void onCreateGroupClicked();
    void onMailPresetChanged(int index);
    void onSaveMailConfigClicked();
    void onSendTestMailClicked();
    void refreshAllTables();

private:
    void buildUi();

    UserSession m_adminSession;
    QWidget *m_rootWidget;
    QTabWidget *m_tabs;

    // Users tab
    QLineEdit *m_editNewUserFullName;
    QLineEdit *m_editNewUserLogin;
    QLineEdit *m_editNewUserEmail;
    QPushButton *m_btnAddUser;
    QTableWidget *m_tblUsers;

    // Groups tab
    QLineEdit *m_editGroupName;
    QLineEdit *m_editGroupDesc;
    QPushButton *m_btnAddGroup;
    QTableWidget *m_tblGroups;

    // Hosts tab
    QTableWidget *m_tblHosts;

    // Mail Server tab
    QComboBox *m_comboMailPreset;
    QLineEdit *m_editSmtpHost;
    QSpinBox *m_spinSmtpPort;
    QComboBox *m_comboEncryption;
    QLineEdit *m_editSmtpUser;
    QLineEdit *m_editSmtpPass;
    QLineEdit *m_editSenderName;
    QLineEdit *m_editSenderEmail;
    QPushButton *m_btnSaveMail;
    QPushButton *m_btnTestMail;

    QAction *m_actRefresh;
};

#endif // ADMINMODULE_H
