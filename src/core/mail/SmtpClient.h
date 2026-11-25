#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QString>

struct MailServerConfig {
    QString providerPreset;   // "GMAIL", "OUTLOOK_MSN", "YAHOO", "CUSTOM"
    QString smtpHost;
    int smtpPort = 587;
    QString encryptionMode;   // "NONE", "STARTTLS", "SSL_TLS"
    QString smtpUser;
    QString smtpPassword;
    QString senderName;
    QString senderEmail;

    bool isValid() const {
        return !smtpHost.trimmed().isEmpty() && smtpPort > 0 && !senderEmail.trimmed().isEmpty();
    }
};

class SmtpClient
{
public:
    static bool sendMail(const MailServerConfig &config,
                         const QString &recipientEmail,
                         const QString &subject,
                         const QString &bodyText,
                         QString &errorOut);

    static bool sendWelcomeCredentialsEmail(const MailServerConfig &config,
                                            const QString &fullName,
                                            const QString &username,
                                            const QString &recipientEmail,
                                            const QString &tempPassword,
                                            QString &errorOut);

    static bool sendPasswordResetEmail(const MailServerConfig &config,
                                       const QString &fullName,
                                       const QString &username,
                                       const QString &recipientEmail,
                                       const QString &tempPassword,
                                       QString &errorOut);
};

#endif // SMTPCLIENT_H
