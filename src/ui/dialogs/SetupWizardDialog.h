#ifndef SETUPWIZARDDIALOG_H
#define SETUPWIZARDDIALOG_H

#include <QDialog>
#include "BootstrapManager.h"

class QStackedWidget;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;

class SetupWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetupWizardDialog(QWidget *parent = nullptr);
    BootstrapConfig completedConfig() const;

private slots:
    void onTestConnectionClicked();
    void onProvisionAndNextClicked();
    void onCreateInitialAccountClicked();

private:
    void setupUi();

    QStackedWidget *m_stack;

    // Page 0: Remote MariaDB Connection
    QLineEdit *m_editHost;
    QSpinBox *m_spinPort;
    QLineEdit *m_editDbName;
    QLineEdit *m_editDbUser;
    QLineEdit *m_editDbPass;
    QLabel *m_lblConnStatus;
    QPushButton *m_btnTestConn;
    QPushButton *m_btnNextProvision;

    // Page 1: Host Assignment & Account Creation (Admin on #1, Standard User on #2+)
    QLabel *m_lblAssignedHostInfo;
    QLabel *m_lblAccountRoleNotice;
    QLineEdit *m_editFullName;
    QLineEdit *m_editUsername;
    QLineEdit *m_editEmail;
    QLabel *m_lblAdminPass;
    QLineEdit *m_editAdminPass;
    QLabel *m_lblAdminPassConfirm;
    QLineEdit *m_editAdminPassConfirm;
    QPushButton *m_btnFinishSetup;

    bool m_connectionTestedOk;
    bool m_isFirstAdminSetup;
    BootstrapConfig m_config;
};

#endif // SETUPWIZARDDIALOG_H
