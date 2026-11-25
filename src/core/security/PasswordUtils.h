#ifndef PASSWORDUTILS_H
#define PASSWORDUTILS_H

#include <QString>

class PasswordUtils
{
public:
    static QString generateRandomPassword(int length = 12);
    static QString hashPassword(const QString &plainPassword);
    static bool verifyPassword(const QString &plainPassword, const QString &storedHash);
};

#endif // PASSWORDUTILS_H
