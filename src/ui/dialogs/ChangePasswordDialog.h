#ifndef CHANGEPASSWORDDIALOG_H
#define CHANGEPASSWORDDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;

class ChangePasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePasswordDialog(const QString &userId, const QString &username, QWidget *parent = nullptr);

private slots:
    void onSavePasswordClicked();

private:
    QString m_userId;
    QLineEdit *m_editNewPassword;
    QLineEdit *m_editConfirmPassword;
    QPushButton *m_btnSave;
};

#endif // CHANGEPASSWORDDIALOG_H
