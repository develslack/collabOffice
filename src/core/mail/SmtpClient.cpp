#include "SmtpClient.h"

#include <QSslSocket>
#include <QByteArray>
#include <QDateTime>
#include <QObject>

namespace {

bool waitForSmtpCode(QSslSocket &socket, int expectedCodePrefix, QString &errorOut, int timeoutMs = 15000)
{
    QByteArray buffer;
    while (socket.waitForReadyRead(timeoutMs)) {
        buffer.append(socket.readAll());
        const QList<QByteArray> lines = buffer.split('\n');
        for (const QByteArray &rawLine : lines) {
            const QByteArray line = rawLine.trimmed();
            if (line.size() >= 4 && line.at(3) == ' ') {
                const int code = line.left(3).toInt();
                if (code / 100 == expectedCodePrefix / 100) {
                    return true;
                }
                errorOut = QString::fromUtf8(line);
                return false;
            }
        }
    }
    errorOut = QObject::tr("SMTP server response timeout (%1).").arg(socket.errorString());
    return false;
}

bool sendSmtpCommand(QSslSocket &socket, const QByteArray &cmd, int expectedCode, QString &errorOut)
{
    socket.write(cmd + "\r\n");
    if (!socket.waitForBytesWritten(10000)) {
        errorOut = QObject::tr("Failed writing data to SMTP server: %1").arg(socket.errorString());
        return false;
    }
    return waitForSmtpCode(socket, expectedCode, errorOut);
}

} // namespace

bool SmtpClient::sendMail(const MailServerConfig &config,
                          const QString &recipientEmail,
                          const QString &subject,
                          const QString &bodyText,
                          QString &errorOut)
{
    if (!config.isValid()) {
        errorOut = QObject::tr("Corporate email server is not configured.");
        return false;
    }

    QSslSocket socket;
    const QString mode = config.encryptionMode.trimmed().toUpper();

    if (mode == QStringLiteral("SSL_TLS")) {
        socket.connectToHostEncrypted(config.smtpHost, static_cast<quint16>(config.smtpPort));
        if (!socket.waitForEncrypted(15000)) {
            errorOut = QObject::tr("SSL/TLS connection failed: %1").arg(socket.errorString());
            return false;
        }
    } else {
        socket.connectToHost(config.smtpHost, static_cast<quint16>(config.smtpPort));
        if (!socket.waitForConnected(15000)) {
            errorOut = QObject::tr("Could not connect to SMTP host: %1").arg(socket.errorString());
            return false;
        }
    }

    // 220 Service ready
    if (!waitForSmtpCode(socket, 220, errorOut)) {
        return false;
    }

    if (!sendSmtpCommand(socket, "EHLO collaboffice.local", 250, errorOut)) {
        return false;
    }

    if (mode == QStringLiteral("STARTTLS")) {
        if (!sendSmtpCommand(socket, "STARTTLS", 220, errorOut)) {
            return false;
        }
        socket.startClientEncryption();
        if (!socket.waitForEncrypted(15000)) {
            errorOut = QObject::tr("STARTTLS handshake failed: %1").arg(socket.errorString());
            return false;
        }
        if (!sendSmtpCommand(socket, "EHLO collaboffice.local", 250, errorOut)) {
            return false;
        }
    }

    // Optional SMTP Authentication
    if (!config.smtpUser.trimmed().isEmpty()) {
        if (!sendSmtpCommand(socket, "AUTH LOGIN", 334, errorOut)) {
            return false;
        }
        if (!sendSmtpCommand(socket, config.smtpUser.toUtf8().toBase64(), 334, errorOut)) {
            return false;
        }
        if (!sendSmtpCommand(socket, config.smtpPassword.toUtf8().toBase64(), 235, errorOut)) {
            return false;
        }
    }

    const QByteArray fromAddr = config.senderEmail.trimmed().toUtf8();
    const QByteArray toAddr = recipientEmail.trimmed().toUtf8();

    if (!sendSmtpCommand(socket, "MAIL FROM:<" + fromAddr + ">", 250, errorOut)) {
        return false;
    }
    if (!sendSmtpCommand(socket, "RCPT TO:<" + toAddr + ">", 250, errorOut)) {
        return false;
    }
    if (!sendSmtpCommand(socket, "DATA", 354, errorOut)) {
        return false;
    }

    const QByteArray encodedSubject = "=?UTF-8?B?" + subject.toUtf8().toBase64() + "?=";
    const QString senderDisplay = config.senderName.isEmpty() ? QStringLiteral("CollabOffice") : config.senderName;

    QByteArray payload;
    payload += "From: \"" + senderDisplay.toUtf8() + "\" <" + fromAddr + ">\r\n";
    payload += "To: <" + toAddr + ">\r\n";
    payload += "Subject: " + encodedSubject + "\r\n";
    payload += "Date: " + QDateTime::currentDateTimeUtc().toString(Qt::RFC2822Date).toUtf8() + "\r\n";
    payload += "MIME-Version: 1.0\r\n";
    payload += "Content-Type: text/plain; charset=UTF-8\r\n";
    payload += "Content-Transfer-Encoding: 8bit\r\n\r\n";
    payload += bodyText.toUtf8() + "\r\n.\r\n";

    socket.write(payload);
    if (!socket.waitForBytesWritten(15000)) {
        errorOut = QObject::tr("Failed transmitting email message body.");
        return false;
    }

    if (!waitForSmtpCode(socket, 250, errorOut)) {
        return false;
    }

    sendSmtpCommand(socket, "QUIT", 221, errorOut);
    socket.disconnectFromHost();
    return true;
}

bool SmtpClient::sendWelcomeCredentialsEmail(const MailServerConfig &config,
                                             const QString &fullName,
                                             const QString &username,
                                             const QString &recipientEmail,
                                             const QString &tempPassword,
                                             QString &errorOut)
{
    const QString subject = QObject::tr("CollabOffice - Your New Account Credentials");
    const QString body = QObject::tr(
        "Hello %1,\n\n"
        "Your user account in CollabOffice has been created.\n\n"
        "Username: %2\n"
        "Temporary Password: %3\n\n"
        "For security reasons, the application will ask you to set a new personal password on your first login.\n\n"
        "Regards,\n"
        "CollabOffice Administration"
    ).arg(fullName, username, tempPassword);

    return sendMail(config, recipientEmail, subject, body, errorOut);
}

bool SmtpClient::sendPasswordResetEmail(const MailServerConfig &config,
                                        const QString &fullName,
                                        const QString &username,
                                        const QString &recipientEmail,
                                        const QString &tempPassword,
                                        QString &errorOut)
{
    const QString subject = QObject::tr("CollabOffice - Password Recovery");
    const QString body = QObject::tr(
        "Hello %1,\n\n"
        "A password reset was requested for your CollabOffice account.\n\n"
        "Username: %2\n"
        "New Temporary Password: %3\n\n"
        "Please log in with this temporary password; you will be prompted to choose a new password immediately.\n\n"
        "Regards,\n"
        "CollabOffice Support"
    ).arg(fullName, username, tempPassword);

    return sendMail(config, recipientEmail, subject, body, errorOut);
}
