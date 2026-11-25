#include "LoginDialog.h"
#include "SmtpClient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>

LoginDialog::LoginDialog(const QString &hostLabel, QWidget *parent)
    : QDialog(parent)
    , m_hostLabel(hostLabel)
{
    setWindowTitle(tr("CollabOffice - Sign In (%1)").arg(m_hostLabel));
    setMinimumWidth(400);
    setModal(true);

    QVBoxLayout *vbox = new QVBoxLayout(this);

    QLabel *lblHeader = new QLabel(tr("<h3>Sign in to CollabOffice</h3>"
                                      "<p>Terminal: <b>%1</b></p>").arg(m_hostLabel), this);
    vbox->addWidget(lblHeader);

    QFormLayout *form = new QFormLayout();
    m_editUserOrEmail = new QLineEdit(this);
    m_editPassword = new QLineEdit(this);
    m_editPassword->setEchoMode(QLineEdit::Password);

    form->addRow(tr("Username or Email:"), m_editUserOrEmail);
    form->addRow(tr("Password:"), m_editPassword);
    vbox->addLayout(form);

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_btnForgotPassword = new QPushButton(tr("Forgot my password"), this);
    m_btnForgotPassword->setFlat(true);
    m_btnLogin = new QPushButton(tr("Sign In"), this);
    m_btnLogin->setDefault(true);

    btnRow->addWidget(m_btnForgotPassword);
    btnRow->addStretch(1);
    btnRow->addWidget(m_btnLogin);
    vbox->addLayout(btnRow);

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_btnForgotPassword, &QPushButton::clicked, this, &LoginDialog::onForgotPasswordClicked);
}

UserSession LoginDialog::authenticatedSession() const
{
    return m_session;
}

void LoginDialog::onLoginClicked()
{
    QString err;
    if (DatabaseManager::instance().authenticateUser(m_editUserOrEmail->text(), m_editPassword->text(), m_session, err)) {
        accept();
    } else {
        QMessageBox::warning(this, tr("Authentication Failed"), err);
        m_editPassword->clear();
        m_editPassword->setFocus();
    }
}

void LoginDialog::onForgotPasswordClicked()
{
    bool ok = false;
    const QString email = QInputDialog::getText(
        this,
        tr("Password Recovery"),
        tr("Enter your registered corporate email address:"),
        QLineEdit::Normal,
        QString(),
        &ok).trimmed();

    if (!ok || email.isEmpty()) {
        return;
    }

    QString fullName;
    QString username;
    QString tempPassword;
    QString err;

    DatabaseManager &dbMgr = DatabaseManager::instance();
    if (!dbMgr.resetPasswordForEmail(email, fullName, username, tempPassword, err)) {
        QMessageBox::warning(this, tr("Password Recovery"), err);
        return;
    }

    const MailServerConfig mailCfg = dbMgr.loadMailServerConfig();
    QString mailErr;
    if (SmtpClient::sendPasswordResetEmail(mailCfg, fullName, username, email, tempPassword, mailErr)) {
        QMessageBox::information(this, tr("Password Sent"),
                                 tr("A new temporary password has been sent to %1.").arg(email));
    } else {
        QMessageBox::warning(this, tr("Email Delivery Error"),
                             tr("Password was reset in the database, but sending email failed: %1").arg(mailErr));
    }
}
