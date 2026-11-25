#include "PasswordUtils.h"

#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QUuid>
#include <QByteArray>
#include <QStringList>

QString PasswordUtils::generateRandomPassword(int length)
{
    const QString upper = QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZ");
    const QString lower = QStringLiteral("abcdefghijkmnopqrstuvwxyz");
    const QString digits = QStringLiteral("23456789");
    const QString symbols = QStringLiteral("@#$%&*!");
    const QString allChars = upper + lower + digits + symbols;

    QRandomGenerator *rng = QRandomGenerator::global();
    QString result;
    result.reserve(length);

    // Guarantee at least one character of each class
    result.append(upper.at(rng->bounded(upper.size())));
    result.append(lower.at(rng->bounded(lower.size())));
    result.append(digits.at(rng->bounded(digits.size())));
    result.append(symbols.at(rng->bounded(symbols.size())));

    for (int i = 4; i < length; ++i) {
        result.append(allChars.at(rng->bounded(allChars.size())));
    }

    // Fisher-Yates shuffle
    for (int i = result.size() - 1; i > 0; --i) {
        const int j = rng->bounded(i + 1);
        const QChar tmp = result[i];
        result[i] = result[j];
        result[j] = tmp;
    }

    return result;
}

QString PasswordUtils::hashPassword(const QString &plainPassword)
{
    const QString salt = QUuid::createUuid().toString(QUuid::WithoutBraces).left(16);
    QByteArray saltedBytes = (salt + QStringLiteral(":") + plainPassword).toUtf8();

    // Key stretching (10,000 SHA-256 rounds)
    for (int i = 0; i < 10000; ++i) {
        saltedBytes = QCryptographicHash::hash(saltedBytes, QCryptographicHash::Sha256);
    }

    return QStringLiteral("sha256$%1$%2").arg(salt, QString::fromLatin1(saltedBytes.toHex()));
}

bool PasswordUtils::verifyPassword(const QString &plainPassword, const QString &storedHash)
{
    const QStringList parts = storedHash.split(QLatin1Char('$'));
    if (parts.size() != 3 || parts.at(0) != QStringLiteral("sha256")) {
        return false;
    }

    const QString &salt = parts.at(1);
    const QString &expectedHex = parts.at(2);

    QByteArray saltedBytes = (salt + QStringLiteral(":") + plainPassword).toUtf8();
    for (int i = 0; i < 10000; ++i) {
        saltedBytes = QCryptographicHash::hash(saltedBytes, QCryptographicHash::Sha256);
    }

    return (QString::fromLatin1(saltedBytes.toHex()) == expectedHex);
}
