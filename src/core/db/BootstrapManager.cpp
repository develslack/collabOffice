#include "BootstrapManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

QString BootstrapManager::bootstrapFilePath()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return configDir + QStringLiteral("/bootstrap.ini");
}

bool BootstrapManager::isBootstrapped()
{
    if (!QFileInfo::exists(bootstrapFilePath())) {
        return false;
    }
    return loadConfig().isConfigured();
}

BootstrapConfig BootstrapManager::loadConfig()
{
    BootstrapConfig cfg;
    QSettings ini(bootstrapFilePath(), QSettings::IniFormat);

    ini.beginGroup(QStringLiteral("Database"));
    cfg.dbHost = ini.value(QStringLiteral("Host"), QStringLiteral("127.0.0.1")).toString();
    cfg.dbPort = ini.value(QStringLiteral("Port"), 3306).toInt();
    cfg.dbName = ini.value(QStringLiteral("DatabaseName"), QStringLiteral("collabOffice")).toString();
    cfg.dbUser = ini.value(QStringLiteral("User")).toString();
    cfg.dbPassword = ini.value(QStringLiteral("Password")).toString();
    ini.endGroup();

    ini.beginGroup(QStringLiteral("Terminal"));
    cfg.hostId = ini.value(QStringLiteral("HostId")).toString();
    cfg.hostNumber = ini.value(QStringLiteral("HostNumber"), 0).toInt();
    cfg.hostLabel = ini.value(QStringLiteral("HostLabel")).toString();
    ini.endGroup();

    return cfg;
}

bool BootstrapManager::saveConfig(const BootstrapConfig &config)
{
    QSettings ini(bootstrapFilePath(), QSettings::IniFormat);

    ini.beginGroup(QStringLiteral("Database"));
    ini.setValue(QStringLiteral("Host"), config.dbHost);
    ini.setValue(QStringLiteral("Port"), config.dbPort);
    ini.setValue(QStringLiteral("DatabaseName"), config.dbName);
    ini.setValue(QStringLiteral("User"), config.dbUser);
    ini.setValue(QStringLiteral("Password"), config.dbPassword);
    ini.endGroup();

    ini.beginGroup(QStringLiteral("Terminal"));
    ini.setValue(QStringLiteral("HostId"), config.hostId);
    ini.setValue(QStringLiteral("HostNumber"), config.hostNumber);
    ini.setValue(QStringLiteral("HostLabel"), config.hostLabel);
    ini.endGroup();

    ini.sync();
    return (ini.status() == QSettings::NoError);
}
