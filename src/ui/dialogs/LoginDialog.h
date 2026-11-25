#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "DatabaseManager.h"

class QLineEdit;
class QPushButton;
class QLabel;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(const QString &hostLabel, QWidget *parent = nullptr);
    UserSession authenticatedSession() const;

private slots:
    void onLoginClicked();
    void onForgotPasswordClicked();

private:
    QString m_hostLabel;
    QLineEdit *m_editUserOrEmail;
    QLineEdit *m_editPassword;
    QPushButton *m_btnLogin;
    QPushButton *m_btnForgotPassword;
    UserSession m_session;
};

#endif // LOGINDIALOG_H
