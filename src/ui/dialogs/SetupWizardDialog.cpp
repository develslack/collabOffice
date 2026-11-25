#include "SetupWizardDialog.h"
#include "DatabaseManager.h"
#include "SmtpClient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QHostInfo>

SetupWizardDialog::SetupWizardDialog(QWidget *parent)
    : QDialog(parent)
    , m_stack(nullptr)
    , m_connectionTestedOk(false)
    , m_isFirstAdminSetup(true)
{
    setupUi();
}

BootstrapConfig SetupWizardDialog::completedConfig() const
{
    return m_config;
}

void SetupWizardDialog::setupUi()
{
    setWindowTitle(tr("CollabOffice - Initial Workstation Setup"));
    setMinimumWidth(520);
    setModal(true);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    m_stack = new QStackedWidget(this);
    rootLayout->addWidget(m_stack);

    // =========================================================================
    // PAGE 0: Remote MariaDB Connection & Database Provisioning
    // =========================================================================
    QWidget *pageDb = new QWidget(this);
    QVBoxLayout *dbVBox = new QVBoxLayout(pageDb);

    QLabel *lblDbTitle = new QLabel(tr("<h3>Step 1: Remote MariaDB Server Connection</h3>"
                                       "<p>Enter the connection details for the central MariaDB server. "
                                       "If the <b>collabOffice</b> database does not exist, it will be created automatically.</p>"), pageDb);
    lblDbTitle->setWordWrap(true);
    dbVBox->addWidget(lblDbTitle);

    QFormLayout *dbForm = new QFormLayout();
    m_editHost = new QLineEdit(QStringLiteral("127.0.0.1"), pageDb);
    m_spinPort = new QSpinBox(pageDb);
    m_spinPort->setRange(1, 65535);
    m_spinPort->setValue(3306);
    m_editDbName = new QLineEdit(QStringLiteral("collabOffice"), pageDb);
    m_editDbName->setReadOnly(true);
    m_editDbUser = new QLineEdit(pageDb);
    m_editDbPass = new QLineEdit(pageDb);
    m_editDbPass->setEchoMode(QLineEdit::Password);

    dbForm->addRow(tr("Server IP / Host:"), m_editHost);
    dbForm->addRow(tr("Port:"), m_spinPort);
    dbForm->addRow(tr("Database Name:"), m_editDbName);
    dbForm->addRow(tr("MariaDB User:"), m_editDbUser);
    dbForm->addRow(tr("MariaDB Password:"), m_editDbPass);
    dbVBox->addLayout(dbForm);

    m_lblConnStatus = new QLabel(pageDb);
    m_lblConnStatus->setWordWrap(true);
    dbVBox->addWidget(m_lblConnStatus);

    QHBoxLayout *dbBtnRow = new QHBoxLayout();
    m_btnTestConn = new QPushButton(tr("Test Connection"), pageDb);
    m_btnNextProvision = new QPushButton(tr("Initialize Database && Continue"), pageDb);
    m_btnNextProvision->setEnabled(false);
    dbBtnRow->addWidget(m_btnTestConn);
    dbBtnRow->addStretch(1);
    dbBtnRow->addWidget(m_btnNextProvision);
    dbVBox->addLayout(dbBtnRow);

    m_stack->addWidget(pageDb);

    // =========================================================================
    // PAGE 1: Terminal Registration & Admin / Standard User Creation
    // =========================================================================
    QWidget *pageAccount = new QWidget(this);
    QVBoxLayout *accVBox = new QVBoxLayout(pageAccount);

    m_lblAssignedHostInfo = new QLabel(pageAccount);
    m_lblAssignedHostInfo->setWordWrap(true);
    accVBox->addWidget(m_lblAssignedHostInfo);

    m_lblAccountRoleNotice = new QLabel(pageAccount);
    m_lblAccountRoleNotice->setWordWrap(true);
    accVBox->addWidget(m_lblAccountRoleNotice);

    QFormLayout *accForm = new QFormLayout();
    m_editFullName = new QLineEdit(pageAccount);
    m_editUsername = new QLineEdit(pageAccount);
    m_editEmail = new QLineEdit(pageAccount);

    m_lblAdminPass = new QLabel(tr("Administrator Password:"), pageAccount);
    m_editAdminPass = new QLineEdit(pageAccount);
    m_editAdminPass->setEchoMode(QLineEdit::Password);

    m_lblAdminPassConfirm = new QLabel(tr("Confirm Password:"), pageAccount);
    m_editAdminPassConfirm = new QLineEdit(pageAccount);
    m_editAdminPassConfirm->setEchoMode(QLineEdit::Password);

    accForm->addRow(tr("Full Name:"), m_editFullName);
    accForm->addRow(tr("Username:"), m_editUsername);
    accForm->addRow(tr("Corporate Email:"), m_editEmail);
    accForm->addRow(m_lblAdminPass, m_editAdminPass);
    accForm->addRow(m_lblAdminPassConfirm, m_editAdminPassConfirm);
    accVBox->addLayout(accForm);

    QHBoxLayout *accBtnRow = new QHBoxLayout();
    accBtnRow->addStretch(1);
    m_btnFinishSetup = new QPushButton(tr("Complete Setup && Go to Login"), pageAccount);
    accBtnRow->addWidget(m_btnFinishSetup);
    accVBox->addLayout(accBtnRow);

    m_stack->addWidget(pageAccount);

    connect(m_btnTestConn, &QPushButton::clicked, this, &SetupWizardDialog::onTestConnectionClicked);
    connect(m_btnNextProvision, &QPushButton::clicked, this, &SetupWizardDialog::onProvisionAndNextClicked);
    connect(m_btnFinishSetup, &QPushButton::clicked, this, &SetupWizardDialog::onCreateInitialAccountClicked);
}

void SetupWizardDialog::onTestConnectionClicked()
{
    QString err;
    const bool ok = DatabaseManager::instance().testMariaDbConnection(
        m_editHost->text(), m_spinPort->value(), m_editDbUser->text(), m_editDbPass->text(), err);

    if (ok) {
        m_connectionTestedOk = true;
        m_btnNextProvision->setEnabled(true);
        m_lblConnStatus->setStyleSheet(QStringLiteral("color: #15803D; font-weight: bold;"));
        m_lblConnStatus->setText(tr("Connection successful! Ready to verify or create the collabOffice database."));
    } else {
        m_connectionTestedOk = false;
        m_btnNextProvision->setEnabled(false);
        m_lblConnStatus->setStyleSheet(QStringLiteral("color: #B91C1C; font-weight: bold;"));
        m_lblConnStatus->setText(tr("Connection failed: %1").arg(err));
    }
}

void SetupWizardDialog::onProvisionAndNextClicked()
{
    if (!m_connectionTestedOk) {
        return;
    }

    QString err;
    DatabaseManager &dbMgr = DatabaseManager::instance();

    if (!dbMgr.provisionDatabaseAndSchema(m_editHost->text(), m_spinPort->value(), m_editDbName->text(),
                                          m_editDbUser->text(), m_editDbPass->text(), err)) {
        QMessageBox::critical(this, tr("Database Provisioning Error"), err);
        return;
    }

    int hostNum = 0;
    QString hostLabel;
    QString hostId;
    const QString machineName = QHostInfo::localHostName();

    if (!dbMgr.registerSequentialHost(machineName, hostNum, hostLabel, hostId, err)) {
        QMessageBox::critical(this, tr("Host Registration Error"), err);
        return;
    }

    m_config.dbHost = m_editHost->text().trimmed();
    m_config.dbPort = m_spinPort->value();
    m_config.dbName = m_editDbName->text().trimmed();
    m_config.dbUser = m_editDbUser->text().trimmed();
    m_config.dbPassword = m_editDbPass->text();
    m_config.hostId = hostId;
    m_config.hostNumber = hostNum;
    m_config.hostLabel = hostLabel;

    BootstrapManager::saveConfig(m_config);

    bool adminExists = false;
    if (!dbMgr.administratorExists(adminExists, err)) {
        QMessageBox::critical(this, tr("Verification Error"), err);
        return;
    }

    m_isFirstAdminSetup = !adminExists;

    m_lblAssignedHostInfo->setText(
        tr("<h3>Step 2: Workstation Registered as <b>%1</b> (Host #%2 - %3)</h3>")
            .arg(hostLabel).arg(hostNum).arg(machineName));

    if (m_isFirstAdminSetup) {
        m_lblAccountRoleNotice->setText(
            tr("This is the first installation. Create the <b>sole System Administrator</b> account. "
               "Future installations on other PCs will only allow creating standard users."));
        m_lblAdminPass->setVisible(true);
        m_editAdminPass->setVisible(true);
        m_lblAdminPassConfirm->setVisible(true);
        m_editAdminPassConfirm->setVisible(true);
    } else {
        m_lblAccountRoleNotice->setText(
            tr("An Administrator already exists in <b>collabOffice</b>. Create a <b>Standard User</b> account for this workstation. "
               "A random password will be generated and sent to the user's email."));
        m_lblAdminPass->setVisible(false);
        m_editAdminPass->setVisible(false);
        m_lblAdminPassConfirm->setVisible(false);
        m_editAdminPassConfirm->setVisible(false);
    }

    m_stack->setCurrentIndex(1);
}

void SetupWizardDialog::onCreateInitialAccountClicked()
{
    const QString fullName = m_editFullName->text().trimmed();
    const QString username = m_editUsername->text().trimmed();
    const QString email = m_editEmail->text().trimmed();

    if (fullName.isEmpty() || username.isEmpty() || !email.contains(QLatin1Char('@'))) {
        QMessageBox::warning(this, tr("Incomplete Data"), tr("Please enter a valid full name, username and email address."));
        return;
    }

    QString err;
    DatabaseManager &dbMgr = DatabaseManager::instance();

    if (m_isFirstAdminSetup) {
        const QString pass = m_editAdminPass->text();
        const QString confirm = m_editAdminPassConfirm->text();
        if (pass.length() < 6 || pass != confirm) {
            QMessageBox::warning(this, tr("Invalid Password"), tr("Password must be at least 6 characters and match confirmation."));
            return;
        }

        if (!dbMgr.createSoleAdministrator(fullName, username, email, pass, m_config.hostId, err)) {
            QMessageBox::critical(this, tr("Error Creating Administrator"), err);
            return;
        }

        QMessageBox::information(this, tr("Administrator Created"),
                                 tr("System Administrator account created successfully.\n"
                                    "Please log in and configure the corporate email server in the Admin Control Panel."));
        accept();
    } else {
        QString generatedPass;
        if (!dbMgr.createStandardUser(fullName, username, email, m_config.hostId, generatedPass, err)) {
            QMessageBox::critical(this, tr("Error Creating User"), err);
            return;
        }

        const MailServerConfig mailCfg = dbMgr.loadMailServerConfig();
        QString mailErr;
        if (SmtpClient::sendWelcomeCredentialsEmail(mailCfg, fullName, username, email, generatedPass, mailErr)) {
            QMessageBox::information(this, tr("User Created"),
                                     tr("User account created. A temporary password has been sent to %1.").arg(email));
        } else {
            QMessageBox::warning(this, tr("User Created (Email Pending)"),
                                 tr("User account was created, but the email could not be sent (%1).\n"
                                    "Ask the Administrator to configure SMTP or provide the temporary password: %2")
                                     .arg(mailErr, generatedPass));
        }
        accept();
    }
}
