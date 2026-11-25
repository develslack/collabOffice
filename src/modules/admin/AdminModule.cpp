#include "AdminModule.h"
#include "SmtpClient.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QStyle>
#include <QApplication>

AdminModule::AdminModule(const UserSession &adminSession, QObject *parent)
    : QObject(parent)
    , m_adminSession(adminSession)
    , m_rootWidget(nullptr)
    , m_tabs(nullptr)
    , m_actRefresh(nullptr)
{
    buildUi();
    refreshAllTables();
}

QString AdminModule::moduleId() const
{
    return QStringLiteral("system_admin_panel");
}

QString AdminModule::displayTitle() const
{
    return tr("Admin Control Panel");
}

QIcon AdminModule::moduleIcon() const
{
    return QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
}

QWidget *AdminModule::centralWidget()
{
    return m_rootWidget;
}

QList<CommandGroupSpec> AdminModule::commandGroups() const
{
    CommandGroupSpec grp;
    grp.tabTitle = tr("Administration");
    grp.groupTitle = tr("Directory & Mail");
    grp.actions = { m_actRefresh };
    return { grp };
}

QString AdminModule::statusSummary() const
{
    return tr("Administrator Control Panel - Manage Users, Groups, Terminals and Corporate SMTP Server");
}

void AdminModule::retranslateUi()
{
    if (m_actRefresh) {
        m_actRefresh->setText(tr("Refresh Lists"));
    }
    if (m_tabs) {
        m_tabs->setTabText(0, tr("Users"));
        m_tabs->setTabText(1, tr("Groups"));
        m_tabs->setTabText(2, tr("Registered Terminals"));
        m_tabs->setTabText(3, tr("Corporate Email (SMTP)"));
    }
}

void AdminModule::buildUi()
{
    m_actRefresh = new QAction(QApplication::style()->standardIcon(QStyle::SP_BrowserReload),
                               tr("Refresh Lists"), this);
    connect(m_actRefresh, &QAction::triggered, this, &AdminModule::refreshAllTables);

    m_rootWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(m_rootWidget);

    m_tabs = new QTabWidget(m_rootWidget);
    mainLayout->addWidget(m_tabs);

    // =========================================================================
    // TAB 0: Users (Passwordless creation -> random password emailed)
    // =========================================================================
    QWidget *tabUsers = new QWidget(m_tabs);
    QVBoxLayout *usersLayout = new QVBoxLayout(tabUsers);

    QGroupBox *grpNewUser = new QGroupBox(tr("Register New User (Password is auto-generated and sent via email)"), tabUsers);
    QHBoxLayout *newUserRow = new QHBoxLayout(grpNewUser);
    m_editNewUserFullName = new QLineEdit(grpNewUser);
    m_editNewUserFullName->setPlaceholderText(tr("Full Name"));
    m_editNewUserLogin = new QLineEdit(grpNewUser);
    m_editNewUserLogin->setPlaceholderText(tr("Username"));
    m_editNewUserEmail = new QLineEdit(grpNewUser);
    m_editNewUserEmail->setPlaceholderText(tr("Email Address"));
    m_btnAddUser = new QPushButton(tr("Create && Send Password"), grpNewUser);

    newUserRow->addWidget(m_editNewUserFullName);
    newUserRow->addWidget(m_editNewUserLogin);
    newUserRow->addWidget(m_editNewUserEmail);
    newUserRow->addWidget(m_btnAddUser);
    usersLayout->addWidget(grpNewUser);

    m_tblUsers = new QTableWidget(0, 6, tabUsers);
    m_tblUsers->setHorizontalHeaderLabels({
        tr("Username"), tr("Full Name"), tr("Email"), tr("Role"), tr("Password Status"), tr("Active")
    });
    m_tblUsers->horizontalHeader()->setStretchLastSection(true);
    m_tblUsers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersLayout->addWidget(m_tblUsers);

    m_tabs->addTab(tabUsers, tr("Users"));

    // =========================================================================
    // TAB 1: Groups
    // =========================================================================
    QWidget *tabGroups = new QWidget(m_tabs);
    QVBoxLayout *groupsLayout = new QVBoxLayout(tabGroups);

    QGroupBox *grpNewGroup = new QGroupBox(tr("Create Department / Security Group"), tabGroups);
    QHBoxLayout *newGroupRow = new QHBoxLayout(grpNewGroup);
    m_editGroupName = new QLineEdit(grpNewGroup);
    m_editGroupName->setPlaceholderText(tr("Group Name (e.g. Finance, HR)"));
    m_editGroupDesc = new QLineEdit(grpNewGroup);
    m_editGroupDesc->setPlaceholderText(tr("Description"));
    m_btnAddGroup = new QPushButton(tr("Create Group"), grpNewGroup);

    newGroupRow->addWidget(m_editGroupName);
    newGroupRow->addWidget(m_editGroupDesc);
    newGroupRow->addWidget(m_btnAddGroup);
    groupsLayout->addWidget(grpNewGroup);

    m_tblGroups = new QTableWidget(0, 3, tabGroups);
    m_tblGroups->setHorizontalHeaderLabels({ tr("Group Name"), tr("Description"), tr("Members") });
    m_tblGroups->horizontalHeader()->setStretchLastSection(true);
    m_tblGroups->setEditTriggers(QAbstractItemView::NoEditTriggers);
    groupsLayout->addWidget(m_tblGroups);

    m_tabs->addTab(tabGroups, tr("Groups"));

    // =========================================================================
    // TAB 2: Registered Terminals (client_hosts)
    // =========================================================================
    QWidget *tabHosts = new QWidget(m_tabs);
    QVBoxLayout *hostsLayout = new QVBoxLayout(tabHosts);
    m_tblHosts = new QTableWidget(0, 4, tabHosts);
    m_tblHosts->setHorizontalHeaderLabels({ tr("Host #"), tr("Terminal ID"), tr("Machine Hostname"), tr("Registered At") });
    m_tblHosts->horizontalHeader()->setStretchLastSection(true);
    m_tblHosts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    hostsLayout->addWidget(m_tblHosts);

    m_tabs->addTab(tabHosts, tr("Registered Terminals"));

    // =========================================================================
    // TAB 3: Corporate Email Engine (SMTP stored in MariaDB for all hosts)
    // =========================================================================
    QWidget *tabMail = new QWidget(m_tabs);
    QVBoxLayout *mailLayout = new QVBoxLayout(tabMail);

    QLabel *lblMailInfo = new QLabel(
        tr("Configure the email delivery engine for the entire organization. "
           "All client terminals read these settings from the central database to send account credentials and notifications."),
        tabMail);
    lblMailInfo->setWordWrap(true);
    mailLayout->addWidget(lblMailInfo);

    QFormLayout *mailForm = new QFormLayout();
    m_comboMailPreset = new QComboBox(tabMail);
    m_comboMailPreset->addItem(tr("Custom / Corporate SMTP Server"), QStringLiteral("CUSTOM"));
    m_comboMailPreset->addItem(QStringLiteral("Gmail (smtp.gmail.com)"), QStringLiteral("GMAIL"));
    m_comboMailPreset->addItem(QStringLiteral("Outlook / MSN / Hotmail (smtp-mail.outlook.com)"), QStringLiteral("OUTLOOK_MSN"));
    m_comboMailPreset->addItem(QStringLiteral("Yahoo Mail (smtp.mail.yahoo.com)"), QStringLiteral("YAHOO"));

    m_editSmtpHost = new QLineEdit(tabMail);
    m_spinSmtpPort = new QSpinBox(tabMail);
    m_spinSmtpPort->setRange(1, 65535);
    m_spinSmtpPort->setValue(587);

    m_comboEncryption = new QComboBox(tabMail);
    m_comboEncryption->addItem(QStringLiteral("STARTTLS (Port 587)"), QStringLiteral("STARTTLS"));
    m_comboEncryption->addItem(QStringLiteral("SSL/TLS (Port 465)"), QStringLiteral("SSL_TLS"));
    m_comboEncryption->addItem(tr("None / Plain (Port 25)"), QStringLiteral("NONE"));

    m_editSmtpUser = new QLineEdit(tabMail);
    m_editSmtpPass = new QLineEdit(tabMail);
    m_editSmtpPass->setEchoMode(QLineEdit::Password);
    m_editSenderName = new QLineEdit(QStringLiteral("CollabOffice"), tabMail);
    m_editSenderEmail = new QLineEdit(tabMail);

    mailForm->addRow(tr("Provider Preset:"), m_comboMailPreset);
    mailForm->addRow(tr("SMTP Server Host:"), m_editSmtpHost);
    mailForm->addRow(tr("SMTP Port:"), m_spinSmtpPort);
    mailForm->addRow(tr("Encryption:"), m_comboEncryption);
    mailForm->addRow(tr("SMTP Username:"), m_editSmtpUser);
    mailForm->addRow(tr("SMTP Password / App Password:"), m_editSmtpPass);
    mailForm->addRow(tr("Sender Display Name:"), m_editSenderName);
    mailForm->addRow(tr("Sender Email Address:"), m_editSenderEmail);
    mailLayout->addLayout(mailForm);

    QHBoxLayout *mailBtnRow = new QHBoxLayout();
    m_btnSaveMail = new QPushButton(tr("Save Mail Configuration"), tabMail);
    m_btnTestMail = new QPushButton(tr("Send Test Email"), tabMail);
    mailBtnRow->addWidget(m_btnSaveMail);
    mailBtnRow->addWidget(m_btnTestMail);
    mailBtnRow->addStretch(1);
    mailLayout->addLayout(mailBtnRow);
    mailLayout->addStretch(1);

    m_tabs->addTab(tabMail, tr("Corporate Email (SMTP)"));

    // Load saved SMTP settings from DB
    const MailServerConfig savedMail = DatabaseManager::instance().loadMailServerConfig();
    if (savedMail.isValid()) {
        const int presetIdx = m_comboMailPreset->findData(savedMail.providerPreset);
        if (presetIdx >= 0) m_comboMailPreset->setCurrentIndex(presetIdx);
        m_editSmtpHost->setText(savedMail.smtpHost);
        m_spinSmtpPort->setValue(savedMail.smtpPort);
        const int encIdx = m_comboEncryption->findData(savedMail.encryptionMode);
        if (encIdx >= 0) m_comboEncryption->setCurrentIndex(encIdx);
        m_editSmtpUser->setText(savedMail.smtpUser);
        m_editSmtpPass->setText(savedMail.smtpPassword);
        m_editSenderName->setText(savedMail.senderName);
        m_editSenderEmail->setText(savedMail.senderEmail);
    }

    connect(m_btnAddUser, &QPushButton::clicked, this, &AdminModule::onCreateUserClicked);
    connect(m_btnAddGroup, &QPushButton::clicked, this, &AdminModule::onCreateGroupClicked);
    connect(m_comboMailPreset, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AdminModule::onMailPresetChanged);
    connect(m_btnSaveMail, &QPushButton::clicked, this, &AdminModule::onSaveMailConfigClicked);
    connect(m_btnTestMail, &QPushButton::clicked, this, &AdminModule::onSendTestMailClicked);
}

void AdminModule::onCreateUserClicked()
{
    const QString fullName = m_editNewUserFullName->text().trimmed();
    const QString username = m_editNewUserLogin->text().trimmed();
    const QString email = m_editNewUserEmail->text().trimmed();

    if (fullName.isEmpty() || username.isEmpty() || !email.contains(QLatin1Char('@'))) {
        QMessageBox::warning(m_rootWidget, tr("Invalid Input"), tr("Enter a valid full name, username and email."));
        return;
    }

    QString generatedPass;
    QString err;
    DatabaseManager &dbMgr = DatabaseManager::instance();

    if (!dbMgr.createStandardUser(fullName, username, email, QString(), generatedPass, err)) {
        QMessageBox::critical(m_rootWidget, tr("User Creation Failed"), err);
        return;
    }

    const MailServerConfig mailCfg = dbMgr.loadMailServerConfig();
    QString mailErr;
    if (SmtpClient::sendWelcomeCredentialsEmail(mailCfg, fullName, username, email, generatedPass, mailErr)) {
        QMessageBox::information(m_rootWidget, tr("User Registered"),
                                 tr("User '%1' created and temporary password emailed to %2.").arg(username, email));
    } else {
        QMessageBox::warning(m_rootWidget, tr("User Registered (Email Failed)"),
                             tr("User '%1' created, but email delivery failed (%2).\nTemporary password: %3")
                                 .arg(username, mailErr, generatedPass));
    }

    m_editNewUserFullName->clear();
    m_editNewUserLogin->clear();
    m_editNewUserEmail->clear();
    refreshAllTables();
}

void AdminModule::onCreateGroupClicked()
{
    const QString name = m_editGroupName->text().trimmed();
    const QString desc = m_editGroupDesc->text().trimmed();
    if (name.isEmpty()) {
        return;
    }

    QString err;
    if (!DatabaseManager::instance().createGroup(name, desc, m_adminSession.userId, err)) {
        QMessageBox::critical(m_rootWidget, tr("Group Creation Failed"), err);
        return;
    }

    m_editGroupName->clear();
    m_editGroupDesc->clear();
    refreshAllTables();
}

void AdminModule::onMailPresetChanged(int index)
{
    const QString preset = m_comboMailPreset->itemData(index).toString();
    if (preset == QStringLiteral("GMAIL")) {
        m_editSmtpHost->setText(QStringLiteral("smtp.gmail.com"));
        m_spinSmtpPort->setValue(587);
        m_comboEncryption->setCurrentIndex(m_comboEncryption->findData(QStringLiteral("STARTTLS")));
    } else if (preset == QStringLiteral("OUTLOOK_MSN")) {
        m_editSmtpHost->setText(QStringLiteral("smtp-mail.outlook.com"));
        m_spinSmtpPort->setValue(587);
        m_comboEncryption->setCurrentIndex(m_comboEncryption->findData(QStringLiteral("STARTTLS")));
    } else if (preset == QStringLiteral("YAHOO")) {
        m_editSmtpHost->setText(QStringLiteral("smtp.mail.yahoo.com"));
        m_spinSmtpPort->setValue(465);
        m_comboEncryption->setCurrentIndex(m_comboEncryption->findData(QStringLiteral("SSL_TLS")));
    }
}

void AdminModule::onSaveMailConfigClicked()
{
    MailServerConfig cfg;
    cfg.providerPreset = m_comboMailPreset->currentData().toString();
    cfg.smtpHost = m_editSmtpHost->text().trimmed();
    cfg.smtpPort = m_spinSmtpPort->value();
    cfg.encryptionMode = m_comboEncryption->currentData().toString();
    cfg.smtpUser = m_editSmtpUser->text().trimmed();
    cfg.smtpPassword = m_editSmtpPass->text();
    cfg.senderName = m_editSenderName->text().trimmed();
    cfg.senderEmail = m_editSenderEmail->text().trimmed();

    QString err;
    if (DatabaseManager::instance().saveMailServerConfig(cfg, err)) {
        QMessageBox::information(m_rootWidget, tr("Configuration Saved"),
                                 tr("Corporate email configuration saved to the central database."));
    } else {
        QMessageBox::critical(m_rootWidget, tr("Save Failed"), err);
    }
}

void AdminModule::onSendTestMailClicked()
{
    bool ok = false;
    const QString targetEmail = QInputDialog::getText(
        m_rootWidget, tr("Send Test Email"), tr("Recipient email address:"),
        QLineEdit::Normal, m_adminSession.email, &ok).trimmed();

    if (!ok || targetEmail.isEmpty()) {
        return;
    }

    MailServerConfig cfg;
    cfg.providerPreset = m_comboMailPreset->currentData().toString();
    cfg.smtpHost = m_editSmtpHost->text().trimmed();
    cfg.smtpPort = m_spinSmtpPort->value();
    cfg.encryptionMode = m_comboEncryption->currentData().toString();
    cfg.smtpUser = m_editSmtpUser->text().trimmed();
    cfg.smtpPassword = m_editSmtpPass->text();
    cfg.senderName = m_editSenderName->text().trimmed();
    cfg.senderEmail = m_editSenderEmail->text().trimmed();

    QString err;
    if (SmtpClient::sendMail(cfg, targetEmail, tr("CollabOffice SMTP Test"),
                             tr("This is a test message confirming your CollabOffice email server configuration."), err)) {
        QMessageBox::information(m_rootWidget, tr("Test Succeeded"), tr("Test email sent successfully to %1.").arg(targetEmail));
    } else {
        QMessageBox::critical(m_rootWidget, tr("Test Failed"), err);
    }
}

void AdminModule::refreshAllTables()
{
    DatabaseManager &dbMgr = DatabaseManager::instance();

    const QList<UserRecord> users = dbMgr.listAllUsers();
    m_tblUsers->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const UserRecord &u = users.at(i);
        m_tblUsers->setItem(i, 0, new QTableWidgetItem(u.username));
        m_tblUsers->setItem(i, 1, new QTableWidgetItem(u.fullName));
        m_tblUsers->setItem(i, 2, new QTableWidgetItem(u.email));
        m_tblUsers->setItem(i, 3, new QTableWidgetItem(u.systemRole));
        m_tblUsers->setItem(i, 4, new QTableWidgetItem(u.mustChangePassword ? tr("Pending First Login") : tr("Active Password")));
        m_tblUsers->setItem(i, 5, new QTableWidgetItem(u.isActive ? tr("Yes") : tr("No")));
    }

    const QList<GroupRecord> groups = dbMgr.listAllGroups();
    m_tblGroups->setRowCount(groups.size());
    for (int i = 0; i < groups.size(); ++i) {
        const GroupRecord &g = groups.at(i);
        m_tblGroups->setItem(i, 0, new QTableWidgetItem(g.name));
        m_tblGroups->setItem(i, 1, new QTableWidgetItem(g.description));
        m_tblGroups->setItem(i, 2, new QTableWidgetItem(QString::number(g.memberCount)));
    }

    const QList<HostRecord> hosts = dbMgr.listAllHosts();
    m_tblHosts->setRowCount(hosts.size());
    for (int i = 0; i < hosts.size(); ++i) {
        const HostRecord &h = hosts.at(i);
        m_tblHosts->setItem(i, 0, new QTableWidgetItem(QString::number(h.hostNumber)));
        m_tblHosts->setItem(i, 1, new QTableWidgetItem(h.hostLabel));
        m_tblHosts->setItem(i, 2, new QTableWidgetItem(h.machineName));
        m_tblHosts->setItem(i, 3, new QTableWidgetItem(h.registeredAt));
    }
}
