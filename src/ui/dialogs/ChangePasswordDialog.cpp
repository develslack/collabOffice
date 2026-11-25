#include "ChangePasswordDialog.h"
#include "DatabaseManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

ChangePasswordDialog::ChangePasswordDialog(const QString &userId, const QString &username, QWidget *parent)
    : QDialog(parent)
    , m_userId(userId)
{
    setWindowTitle(tr("CollabOffice - Password Update Required"));
    setMinimumWidth(420);
    setModal(true);

    QVBoxLayout *vbox = new QVBoxLayout(this);

    QLabel *lblNotice = new QLabel(
        tr("<h3>First Login Security Check</h3>"
           "<p>Welcome <b>%1</b>. Because your account uses a temporary password, "
           "you must set a new personal password before continuing.</p>").arg(username), this);
    lblNotice->setWordWrap(true);
    vbox->addWidget(lblNotice);

    QFormLayout *form = new QFormLayout();
    m_editNewPassword = new QLineEdit(this);
    m_editNewPassword->setEchoMode(QLineEdit::Password);
    m_editConfirmPassword = new QLineEdit(this);
    m_editConfirmPassword->setEchoMode(QLineEdit::Password);

    form->addRow(tr("New Password:"), m_editNewPassword);
    form->addRow(tr("Confirm New Password:"), m_editConfirmPassword);
    vbox->addLayout(form);

    m_btnSave = new QPushButton(tr("Update Password && Sign In Again"), this);
    vbox->addWidget(m_btnSave);

    connect(m_btnSave, &QPushButton::clicked, this, &ChangePasswordDialog::onSavePasswordClicked);
}

void ChangePasswordDialog::onSavePasswordClicked()
{
    const QString newPass = m_editNewPassword->text();
    const QString confirm = m_editConfirmPassword->text();

    if (newPass.length() < 6) {
        QMessageBox::warning(this, tr("Weak Password"), tr("The new password must be at least 6 characters long."));
        return;
    }
    if (newPass != confirm) {
        QMessageBox::warning(this, tr("Mismatch"), tr("Password and confirmation do not match."));
        return;
    }

    QString err;
    if (!DatabaseManager::instance().changeUserPassword(m_userId, newPass, err)) {
        QMessageBox::critical(this, tr("Error"), err);
        return;
    }

    QMessageBox::information(this, tr("Password Updated"),
                             tr("Your password has been updated. You will now be logged out so you can sign in with your new password."));
    accept();
}
