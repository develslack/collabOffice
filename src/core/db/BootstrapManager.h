#ifndef BOOTSTRAPMANAGER_H
#define BOOTSTRAPMANAGER_H

#include <QString>

struct BootstrapConfig {
    QString dbHost = QStringLiteral("127.0.0.1");
    int dbPort = 3306;
    QString dbName = QStringLiteral("collabOffice");
    QString dbUser;
    QString dbPassword;

    QString hostId;
    int hostNumber = 0;
    QString hostLabel;

    bool isConfigured() const {
        return !dbHost.isEmpty() && !dbName.isEmpty() && !dbUser.isEmpty() && !hostId.isEmpty() && hostNumber > 0;
    }
};

class BootstrapManager
{
public:
    static QString bootstrapFilePath();
    static bool isBootstrapped();
    static BootstrapConfig loadConfig();
    static bool saveConfig(const BootstrapConfig &config);
};

#endif // BOOTSTRAPMANAGER_H
