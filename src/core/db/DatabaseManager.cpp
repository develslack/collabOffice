#include "DatabaseManager.h"
#include "PasswordUtils.h"
#include "QtCompat.h"

#include <QMap>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>


namespace {

QString utcNowIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

} // namespace

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager s_instance;
    return s_instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_remoteOnline(false)
    , m_remoteConnName(QStringLiteral("CollabOffice_MariaDB"))
    , m_localConnName(QStringLiteral("CollabOffice_LocalSQLite"))
{
}

DatabaseManager::~DatabaseManager()
{
    if (QSqlDatabase::contains(m_remoteConnName)) {
        QSqlDatabase::database(m_remoteConnName).close();
        QSqlDatabase::removeDatabase(m_remoteConnName);
    }
    if (QSqlDatabase::contains(m_localConnName)) {
        QSqlDatabase::database(m_localConnName).close();
        QSqlDatabase::removeDatabase(m_localConnName);
    }
}

bool DatabaseManager::testMariaDbConnection(const QString &host, int port,
                                            const QString &user, const QString &password,
                                            QString &errorOut)
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QMYSQL"))) {
        errorOut = tr("Qt MySQL/MariaDB driver (QMYSQL) is not installed on this system.");
        return false;
    }

    const QString testConn = QStringLiteral("CollabOffice_TestConn");
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), testConn);
        db.setHostName(host.trimmed());
        db.setPort(port);
        db.setUserName(user.trimmed());
        db.setPassword(password);
        db.setConnectOptions(QStringLiteral("MYSQL_OPT_CONNECT_TIMEOUT=5"));

        if (db.open()) {
            ok = true;
            db.close();
        } else {
            errorOut = db.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(testConn);
    return ok;
}

bool DatabaseManager::provisionDatabaseAndSchema(const QString &host, int port, const QString &dbName,
                                                 const QString &user, const QString &password,
                                                 QString &errorOut)
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QMYSQL"))) {
        errorOut = tr("Qt MySQL/MariaDB driver (QMYSQL) is not available.");
        return false;
    }

    const QString safeDbName = dbName.trimmed().isEmpty() ? QStringLiteral("collabOffice") : dbName.trimmed();
    const QString initConn = QStringLiteral("CollabOffice_InitConn");

    // Step 1: Connect without database and ensure `collabOffice` database exists
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), initConn);
        db.setHostName(host.trimmed());
        db.setPort(port);
        db.setUserName(user.trimmed());
        db.setPassword(password);

        if (!db.open()) {
            errorOut = db.lastError().text();
            QSqlDatabase::removeDatabase(initConn);
            return false;
        }

        QString escapedName = safeDbName;
        escapedName.replace(QLatin1Char('`'), QStringLiteral("``"));

        QSqlQuery createDbQuery(db);
        const QString sql = QStringLiteral("CREATE DATABASE IF NOT EXISTS `%1` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;")
                                .arg(escapedName);
        if (!createDbQuery.exec(sql)) {
            errorOut = createDbQuery.lastError().text();
            db.close();
            QSqlDatabase::removeDatabase(initConn);
            return false;
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(initConn);

    // Step 2: Open persistent connection to `collabOffice` and execute full DDL schema
    if (QSqlDatabase::contains(m_remoteConnName)) {
        QSqlDatabase::database(m_remoteConnName).close();
        QSqlDatabase::removeDatabase(m_remoteConnName);
    }

    QSqlDatabase remoteDb = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), m_remoteConnName);
    remoteDb.setHostName(host.trimmed());
    remoteDb.setPort(port);
    remoteDb.setDatabaseName(safeDbName);
    remoteDb.setUserName(user.trimmed());
    remoteDb.setPassword(password);

    if (!remoteDb.open()) {
        errorOut = remoteDb.lastError().text();
        return false;
    }

    if (!executeSqlScript(remoteDb, QStringLiteral(":/sql/schema.sql"), errorOut)) {
        return false;
    }

    m_remoteOnline = true;
    emit connectionStatusChanged(true);

    // Step 3: Ensure local SQLite offline mirror also has the schema ready
    QString sqliteErr;
    initializeLocalSqliteMirror(sqliteErr);

    return true;
}

bool DatabaseManager::connectFromBootstrap(const BootstrapConfig &config, QString &errorOut)
{
    QString sqliteErr;
    initializeLocalSqliteMirror(sqliteErr);

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QMYSQL"))) {
        m_remoteOnline = false;
        emit connectionStatusChanged(false);
        errorOut = tr("QMYSQL driver unavailable; running on local SQLite cache.");
        return false;
    }

    if (QSqlDatabase::contains(m_remoteConnName)) {
        QSqlDatabase::database(m_remoteConnName).close();
        QSqlDatabase::removeDatabase(m_remoteConnName);
    }

    QSqlDatabase remoteDb = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), m_remoteConnName);
    remoteDb.setHostName(config.dbHost);
    remoteDb.setPort(config.dbPort);
    remoteDb.setDatabaseName(config.dbName);
    remoteDb.setUserName(config.dbUser);
    remoteDb.setPassword(config.dbPassword);
    remoteDb.setConnectOptions(QStringLiteral("MYSQL_OPT_CONNECT_TIMEOUT=5"));

    if (remoteDb.open()) {
        // Ensure tables exist in case schema was updated
        executeSqlScript(remoteDb, QStringLiteral(":/sql/schema.sql"), errorOut);
        m_remoteOnline = true;
        emit connectionStatusChanged(true);
        updateHostHeartbeat(config.hostId);
        return true;
    }

    m_remoteOnline = false;
    emit connectionStatusChanged(false);
    errorOut = remoteDb.lastError().text();
    return false;
}

bool DatabaseManager::isRemoteOnline() const
{
    return m_remoteOnline;
}

bool DatabaseManager::initializeLocalSqliteMirror(QString &errorOut)
{
    if (QSqlDatabase::contains(m_localConnName)) {
        QSqlDatabase existing = QSqlDatabase::database(m_localConnName);
        if (existing.isOpen()) {
            return true;
        }
    }

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString sqlitePath = dataDir + QStringLiteral("/offline_cache.sqlite");

    QSqlDatabase localDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_localConnName);
    localDb.setDatabaseName(sqlitePath);

    if (!localDb.open()) {
        errorOut = localDb.lastError().text();
        return false;
    }

    return executeSqlScript(localDb, QStringLiteral(":/sql/schema.sql"), errorOut);
}

bool DatabaseManager::executeSqlScript(QSqlDatabase &db, const QString &resourcePath, QString &errorOut)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorOut = tr("Could not open embedded SQL schema: %1").arg(resourcePath);
        return false;
    }

    QTextStream in(&file);
    QtCompat::setUtf8Encoding(in);
    const QString fullScript = in.readAll();

    const QStringList statements = fullScript.split(QLatin1Char(';'), QtCompat::SkipEmptyParts);
    QSqlQuery query(db);

    for (const QString &rawStmt : statements) {
        const QString stmt = rawStmt.trimmed();
        if (stmt.isEmpty() || stmt.startsWith(QStringLiteral("--"))) {
            continue;
        }
        if (!query.exec(stmt)) {
            errorOut = QStringLiteral("SQL Error: %1\nStatement: %2").arg(query.lastError().text(), stmt);
            return false;
        }
    }
    return true;
}

QSqlDatabase DatabaseManager::activeDb() const
{
    if (m_remoteOnline && QSqlDatabase::contains(m_remoteConnName)) {
        QSqlDatabase remote = QSqlDatabase::database(m_remoteConnName);
        if (remote.isOpen()) {
            return remote;
        }
    }
    return QSqlDatabase::database(m_localConnName);
}

bool DatabaseManager::registerSequentialHost(const QString &machineName, int &hostNumberOut,
                                             QString &hostLabelOut, QString &hostIdOut, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Remote MariaDB connection is not open.");
        return false;
    }

    db.transaction();

    QSqlQuery countQuery(db);
    if (!countQuery.exec(QStringLiteral("SELECT COALESCE(MAX(host_number), 0) + 1 FROM client_hosts"))) {
        errorOut = countQuery.lastError().text();
        db.rollback();
        return false;
    }

    int nextNum = 1;
    if (countQuery.next()) {
        nextNum = countQuery.value(0).toInt();
    }

    const QString newHostId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString label = QStringLiteral("PC-%1").arg(nextNum, 3, 10, QLatin1Char('0'));
    const QString now = utcNowIso();

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO client_hosts (id, host_number, host_label, machine_name, registered_at, last_seen_at) "
        "VALUES (:id, :num, :label, :machine, :reg, :seen)"));
    insertQuery.bindValue(QStringLiteral(":id"), newHostId);
    insertQuery.bindValue(QStringLiteral(":num"), nextNum);
    insertQuery.bindValue(QStringLiteral(":label"), label);
    insertQuery.bindValue(QStringLiteral(":machine"), machineName.trimmed());
    insertQuery.bindValue(QStringLiteral(":reg"), now);
    insertQuery.bindValue(QStringLiteral(":seen"), now);

    if (!insertQuery.exec()) {
        errorOut = insertQuery.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        errorOut = db.lastError().text();
        return false;
    }

    hostNumberOut = nextNum;
    hostLabelOut = label;
    hostIdOut = newHostId;
    return true;
}

void DatabaseManager::updateHostHeartbeat(const QString &hostId)
{
    if (!m_remoteOnline || hostId.isEmpty()) {
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE client_hosts SET last_seen_at = :seen WHERE id = :id"));
    q.bindValue(QStringLiteral(":seen"), utcNowIso());
    q.bindValue(QStringLiteral(":id"), hostId);
    q.exec();
}

bool DatabaseManager::administratorExists(bool &existsOut, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Remote database is not connected.");
        return false;
    }

    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM users WHERE system_role = 'ADMIN'"))) {
        errorOut = q.lastError().text();
        return false;
    }

    existsOut = false;
    if (q.next()) {
        existsOut = (q.value(0).toInt() > 0);
    }
    return true;
}

bool DatabaseManager::createSoleAdministrator(const QString &fullName, const QString &username,
                                              const QString &email, const QString &plainPassword,
                                              const QString &hostId, QString &errorOut)
{
    bool adminAlreadyExists = false;
    if (!administratorExists(adminAlreadyExists, errorOut)) {
        return false;
    }
    if (adminAlreadyExists) {
        errorOut = tr("An Administrator account already exists in the database. Only one Administrator is allowed.");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString hash = PasswordUtils::hashPassword(plainPassword);
    const QString now = utcNowIso();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO users (id, username, full_name, email, password_hash, system_role, must_change_password, registered_host_id, is_active, created_at) "
        "VALUES (:id, :user, :name, :email, :hash, 'ADMIN', 0, :hostId, 1, :created)"));
    q.bindValue(QStringLiteral(":id"), id);
    q.bindValue(QStringLiteral(":user"), username.trimmed());
    q.bindValue(QStringLiteral(":name"), fullName.trimmed());
    q.bindValue(QStringLiteral(":email"), email.trimmed().toLower());
    q.bindValue(QStringLiteral(":hash"), hash);
    q.bindValue(QStringLiteral(":hostId"), hostId);
    q.bindValue(QStringLiteral(":created"), now);

    if (!q.exec()) {
        errorOut = q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createStandardUser(const QString &fullName, const QString &username,
                                         const QString &email, const QString &hostId,
                                         QString &generatedPasswordOut, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Cannot create users while offline. Connect to MariaDB first.");
        return false;
    }

    const QString tempPass = PasswordUtils::generateRandomPassword(12);
    const QString hash = PasswordUtils::hashPassword(tempPass);
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString now = utcNowIso();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO users (id, username, full_name, email, password_hash, system_role, must_change_password, registered_host_id, is_active, created_at) "
        "VALUES (:id, :user, :name, :email, :hash, 'USER', 1, :hostId, 1, :created)"));
    q.bindValue(QStringLiteral(":id"), id);
    q.bindValue(QStringLiteral(":user"), username.trimmed());
    q.bindValue(QStringLiteral(":name"), fullName.trimmed());
    q.bindValue(QStringLiteral(":email"), email.trimmed().toLower());
    q.bindValue(QStringLiteral(":hash"), hash);
    q.bindValue(QStringLiteral(":hostId"), hostId.isEmpty() ? QVariant() : QVariant(hostId));
    q.bindValue(QStringLiteral(":created"), now);

    if (!q.exec()) {
        errorOut = q.lastError().text();
        return false;
    }

    generatedPasswordOut = tempPass;
    return true;
}

bool DatabaseManager::authenticateUser(const QString &usernameOrEmail, const QString &plainPassword,
                                       UserSession &sessionOut, QString &errorOut)
{
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        errorOut = tr("No database connection is available.");
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, username, full_name, email, password_hash, system_role, must_change_password, "
        "ui_mode, shortcut_profile, theme, is_active "
        "FROM users WHERE username = :login OR email = :loginLower LIMIT 1"));
    q.bindValue(QStringLiteral(":login"), usernameOrEmail.trimmed());
    q.bindValue(QStringLiteral(":loginLower"), usernameOrEmail.trimmed().toLower());

    if (!q.exec()) {
        errorOut = q.lastError().text();
        return false;
    }

    if (!q.next()) {
        errorOut = tr("Invalid username/email or password.");
        return false;
    }

    if (q.value(10).toInt() == 0) {
        errorOut = tr("This user account is disabled. Contact the system administrator.");
        return false;
    }

    const QString storedHash = q.value(4).toString();
    if (!PasswordUtils::verifyPassword(plainPassword, storedHash)) {
        errorOut = tr("Invalid username/email or password.");
        return false;
    }

    sessionOut.userId = q.value(0).toString();
    sessionOut.username = q.value(1).toString();
    sessionOut.fullName = q.value(2).toString();
    sessionOut.email = q.value(3).toString();
    sessionOut.systemRole = q.value(5).toString();
    sessionOut.mustChangePassword = (q.value(6).toInt() != 0);
    sessionOut.uiMode = q.value(7).toString();
    sessionOut.shortcutProfile = q.value(8).toString();
    sessionOut.theme = q.value(9).toString();

    return true;
}

bool DatabaseManager::changeUserPassword(const QString &userId, const QString &newPlainPassword, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Password changes require an active connection to the MariaDB server.");
        return false;
    }

    const QString newHash = PasswordUtils::hashPassword(newPlainPassword);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE users SET password_hash = :hash, must_change_password = 0 WHERE id = :id"));
    q.bindValue(QStringLiteral(":hash"), newHash);
    q.bindValue(QStringLiteral(":id"), userId);

    if (!q.exec()) {
        errorOut = q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::resetPasswordForEmail(const QString &email, QString &fullNameOut, QString &usernameOut,
                                            QString &tempPasswordOut, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Password recovery requires an active connection to the MariaDB server.");
        return false;
    }

    QSqlQuery findQ(db);
    findQ.prepare(QStringLiteral("SELECT id, username, full_name, is_active FROM users WHERE email = :email LIMIT 1"));
    findQ.bindValue(QStringLiteral(":email"), email.trimmed().toLower());

    if (!findQ.exec() || !findQ.next()) {
        errorOut = tr("No account was found with that email address.");
        return false;
    }

    if (findQ.value(3).toInt() == 0) {
        errorOut = tr("This account is disabled. Contact the administrator.");
        return false;
    }

    const QString userId = findQ.value(0).toString();
    usernameOut = findQ.value(1).toString();
    fullNameOut = findQ.value(2).toString();
    tempPasswordOut = PasswordUtils::generateRandomPassword(12);

    const QString hash = PasswordUtils::hashPassword(tempPasswordOut);
    QSqlQuery updQ(db);
    updQ.prepare(QStringLiteral("UPDATE users SET password_hash = :hash, must_change_password = 1 WHERE id = :id"));
    updQ.bindValue(QStringLiteral(":hash"), hash);
    updQ.bindValue(QStringLiteral(":id"), userId);

    if (!updQ.exec()) {
        errorOut = updQ.lastError().text();
        return false;
    }

    return true;
}

MailServerConfig DatabaseManager::loadMailServerConfig()
{
    MailServerConfig cfg;
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        return cfg;
    }

    QSqlQuery q(db);
    if (q.exec(QStringLiteral(
            "SELECT provider_preset, smtp_host, smtp_port, encryption_mode, smtp_user, smtp_password, sender_name, sender_email "
            "FROM system_mail_config WHERE id = 1")) && q.next()) {
        cfg.providerPreset = q.value(0).toString();
        cfg.smtpHost = q.value(1).toString();
        cfg.smtpPort = q.value(2).toInt();
        cfg.encryptionMode = q.value(3).toString();
        cfg.smtpUser = q.value(4).toString();
        cfg.smtpPassword = q.value(5).toString();
        cfg.senderName = q.value(6).toString();
        cfg.senderEmail = q.value(7).toString();
    }
    return cfg;
}

bool DatabaseManager::saveMailServerConfig(const MailServerConfig &config, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Cannot save mail configuration while offline.");
        return false;
    }

    QSqlQuery delQ(db);
    delQ.exec(QStringLiteral("DELETE FROM system_mail_config WHERE id = 1"));

    QSqlQuery insQ(db);
    insQ.prepare(QStringLiteral(
        "INSERT INTO system_mail_config (id, provider_preset, smtp_host, smtp_port, encryption_mode, smtp_user, smtp_password, sender_name, sender_email, updated_at) "
        "VALUES (1, :preset, :host, :port, :enc, :user, :pass, :sname, :semail, :upd)"));
    insQ.bindValue(QStringLiteral(":preset"), config.providerPreset);
    insQ.bindValue(QStringLiteral(":host"), config.smtpHost.trimmed());
    insQ.bindValue(QStringLiteral(":port"), config.smtpPort);
    insQ.bindValue(QStringLiteral(":enc"), config.encryptionMode);
    insQ.bindValue(QStringLiteral(":user"), config.smtpUser.trimmed());
    insQ.bindValue(QStringLiteral(":pass"), config.smtpPassword);
    insQ.bindValue(QStringLiteral(":sname"), config.senderName.trimmed());
    insQ.bindValue(QStringLiteral(":semail"), config.senderEmail.trimmed());
    insQ.bindValue(QStringLiteral(":upd"), utcNowIso());

    if (!insQ.exec()) {
        errorOut = insQ.lastError().text();
        return false;
    }
    return true;
}

QList<UserRecord> DatabaseManager::listAllUsers()
{
    QList<UserRecord> list;
    QSqlDatabase db = activeDb();
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("SELECT id, username, full_name, email, system_role, must_change_password, is_active, created_at FROM users ORDER BY created_at ASC"))) {
        while (q.next()) {
            UserRecord u;
            u.id = q.value(0).toString();
            u.username = q.value(1).toString();
            u.fullName = q.value(2).toString();
            u.email = q.value(3).toString();
            u.systemRole = q.value(4).toString();
            u.mustChangePassword = (q.value(5).toInt() != 0);
            u.isActive = (q.value(6).toInt() != 0);
            u.createdAt = q.value(7).toString();
            list.append(u);
        }
    }
    return list;
}

QList<HostRecord> DatabaseManager::listAllHosts()
{
    QList<HostRecord> list;
    QSqlDatabase db = activeDb();
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("SELECT id, host_number, host_label, machine_name, registered_at FROM client_hosts ORDER BY host_number ASC"))) {
        while (q.next()) {
            HostRecord h;
            h.id = q.value(0).toString();
            h.hostNumber = q.value(1).toInt();
            h.hostLabel = q.value(2).toString();
            h.machineName = q.value(3).toString();
            h.registeredAt = q.value(4).toString();
            list.append(h);
        }
    }
    return list;
}

QList<GroupRecord> DatabaseManager::listAllGroups()
{
    QList<GroupRecord> list;
    QSqlDatabase db = activeDb();
    QSqlQuery q(db);
    if (q.exec(QStringLiteral(
            "SELECT g.id, g.name, g.description, COUNT(m.user_id) "
            "FROM user_groups g LEFT JOIN user_group_members m ON g.id = m.group_id "
            "GROUP BY g.id, g.name, g.description ORDER BY g.name ASC"))) {
        while (q.next()) {
            GroupRecord g;
            g.id = q.value(0).toString();
            g.name = q.value(1).toString();
            g.description = q.value(2).toString();
            g.memberCount = q.value(3).toInt();
            list.append(g);
        }
    }
    return list;
}

bool DatabaseManager::createGroup(const QString &name, const QString &description, const QString &createdByUserId, QString &errorOut)
{
    QSqlDatabase db = QSqlDatabase::database(m_remoteConnName);
    if (!db.isOpen()) {
        errorOut = tr("Cannot create groups while offline.");
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT INTO user_groups (id, name, description, created_by, created_at) VALUES (:id, :name, :desc, :by, :created)"));
    q.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    q.bindValue(QStringLiteral(":name"), name.trimmed());
    q.bindValue(QStringLiteral(":desc"), description.trimmed());
    q.bindValue(QStringLiteral(":by"), createdByUserId);
    q.bindValue(QStringLiteral(":created"), utcNowIso());

    if (!q.exec()) {
        errorOut = q.lastError().text();
        return false;
    }
    return true;
}


QList<DocumentSummary> DatabaseManager::listAccessibleDocuments(const UserSession &session)
{
    QList<DocumentSummary> result;
    QSqlDatabase db = activeDb();
    if (!db.isOpen() || !session.isValid()) {
        return result;
    }

    QSqlQuery q(db);
    if (session.isAdmin()) {
        // System Administrator sees ALL documents across the organization
        q.prepare(QStringLiteral(
            "SELECT d.id, d.title, d.doc_type, d.owner_id, COALESCE(u.full_name, u.username), "
            "d.current_version, d.updated_at "
            "FROM documents d "
            "LEFT JOIN users u ON d.owner_id = u.id "
            "WHERE d.is_archived = 0 "
            "ORDER BY d.updated_at DESC"));
    } else {
        // Standard User ONLY sees documents where they have direct ownership, user/group ACL, or element ACL
        q.prepare(QStringLiteral(
            "SELECT DISTINCT d.id, d.title, d.doc_type, d.owner_id, COALESCE(u.full_name, u.username), "
            "d.current_version, d.updated_at, "
            "CASE "
            "  WHEN d.owner_id = :uid1 THEN 'OWNER' "
            "  WHEN EXISTS (SELECT 1 FROM document_permissions dp WHERE dp.document_id = d.id AND dp.user_id = :uid2 AND dp.access_level = 'CO_ADMIN') THEN 'CO_ADMIN' "
            "  WHEN EXISTS (SELECT 1 FROM document_permissions dp JOIN user_group_members gm ON dp.group_id = gm.group_id "
            "               WHERE dp.document_id = d.id AND gm.user_id = :uid3 AND dp.access_level = 'CO_ADMIN') THEN 'CO_ADMIN' "
            "  WHEN EXISTS (SELECT 1 FROM document_permissions dp WHERE dp.document_id = d.id AND dp.user_id = :uid4 AND dp.access_level = 'PROPOSE_EDIT') THEN 'PROPOSE_EDIT' "
            "  WHEN EXISTS (SELECT 1 FROM document_permissions dp JOIN user_group_members gm ON dp.group_id = gm.group_id "
            "               WHERE dp.document_id = d.id AND gm.user_id = :uid5 AND dp.access_level = 'PROPOSE_EDIT') THEN 'PROPOSE_EDIT' "
            "  WHEN EXISTS (SELECT 1 FROM element_permissions ep WHERE ep.document_id = d.id AND ep.can_propose_edit = 1 AND "
            "               (ep.user_id = :uid6 OR ep.group_id IN (SELECT group_id FROM user_group_members WHERE user_id = :uid7))) THEN 'PROPOSE_EDIT' "
            "  ELSE 'READ' "
            "END AS eff_access "
            "FROM documents d "
            "LEFT JOIN users u ON d.owner_id = u.id "
            "WHERE d.is_archived = 0 AND ( "
            "  d.owner_id = :uid8 "
            "  OR EXISTS (SELECT 1 FROM document_permissions dp WHERE dp.document_id = d.id AND dp.user_id = :uid9 AND dp.access_level <> 'NONE') "
            "  OR EXISTS (SELECT 1 FROM document_permissions dp JOIN user_group_members gm ON dp.group_id = gm.group_id "
            "             WHERE dp.document_id = d.id AND gm.user_id = :uid10 AND dp.access_level <> 'NONE') "
            "  OR EXISTS (SELECT 1 FROM element_permissions ep WHERE ep.document_id = d.id AND (ep.can_read = 1 OR ep.can_propose_edit = 1) AND "
            "             (ep.user_id = :uid11 OR ep.group_id IN (SELECT group_id FROM user_group_members WHERE user_id = :uid12))) "
            ") "
            "ORDER BY d.updated_at DESC"));

        for (int i = 1; i <= 12; ++i) {
            q.bindValue(QStringLiteral(":uid%1").arg(i), session.userId);
        }
    }

    if (!q.exec()) {
        qWarning() << "[DatabaseManager] listAccessibleDocuments failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        DocumentSummary doc;
        doc.id = q.value(0).toString();
        doc.title = q.value(1).toString();
        doc.docType = q.value(2).toString();
        doc.ownerId = q.value(3).toString();
        doc.ownerName = q.value(4).toString();
        doc.currentVersion = q.value(5).toInt();
        doc.updatedAt = q.value(6).toString();
        if (session.isAdmin()) {
            doc.effectiveAccess = (doc.ownerId == session.userId)
                                      ? QStringLiteral("OWNER")
                                      : QStringLiteral("ADMIN_VIEW");
        } else {
            doc.effectiveAccess = q.value(7).toString();
        }
        result.append(doc);
    }

    return result;
}

bool DatabaseManager::createDocument(const QString &title, const QString &docType, const QString &ownerId,
                                     QString &newDocIdOut, QString &errorOut)
{
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        errorOut = tr("No active database connection.");
        return false;
    }

    const QString docId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString now = utcNowIso();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO documents (id, title, doc_type, owner_id, current_version, is_archived, created_at, updated_at) "
        "VALUES (:id, :title, :type, :owner, 1, 0, :created, :updated)"));
    q.bindValue(QStringLiteral(":id"), docId);
    q.bindValue(QStringLiteral(":title"), title.trimmed());
    q.bindValue(QStringLiteral(":type"), docType);
    q.bindValue(QStringLiteral(":owner"), ownerId);
    q.bindValue(QStringLiteral(":created"), now);
    q.bindValue(QStringLiteral(":updated"), now);

    if (!q.exec()) {
        errorOut = q.lastError().text();
        return false;
    }

    newDocIdOut = docId;
    return true;
}

bool DatabaseManager::grantDocumentPermission(const QString &documentId, const QString &targetUserId,
                                              const QString &targetGroupId, const QString &accessLevel,
                                              const QString &grantedByUserId, QString &errorOut)
{
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        errorOut = tr("No active database connection.");
        return false;
    }

    QSqlQuery delQ(db);
    if (!targetUserId.isEmpty()) {
        delQ.prepare(QStringLiteral("DELETE FROM document_permissions WHERE document_id = :doc AND user_id = :uid"));
        delQ.bindValue(QStringLiteral(":doc"), documentId);
        delQ.bindValue(QStringLiteral(":uid"), targetUserId);
    } else {
        delQ.prepare(QStringLiteral("DELETE FROM document_permissions WHERE document_id = :doc AND group_id = :gid"));
        delQ.bindValue(QStringLiteral(":doc"), documentId);
        delQ.bindValue(QStringLiteral(":gid"), targetGroupId);
    }
    delQ.exec();

    QSqlQuery insQ(db);
    insQ.prepare(QStringLiteral(
        "INSERT INTO document_permissions (id, document_id, user_id, group_id, access_level, granted_by, granted_at) "
        "VALUES (:id, :doc, :uid, :gid, :level, :by, :at)"));
    insQ.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    insQ.bindValue(QStringLiteral(":doc"), documentId);
    insQ.bindValue(QStringLiteral(":uid"), targetUserId.isEmpty() ? QVariant() : QVariant(targetUserId));
    insQ.bindValue(QStringLiteral(":gid"), targetGroupId.isEmpty() ? QVariant() : QVariant(targetGroupId));
    insQ.bindValue(QStringLiteral(":level"), accessLevel);
    insQ.bindValue(QStringLiteral(":by"), grantedByUserId);
    insQ.bindValue(QStringLiteral(":at"), utcNowIso());

    if (!insQ.exec()) {
        errorOut = insQ.lastError().text();
        return false;
    }
    return true;
}

QList<DocBlockRecord> DatabaseManager::loadDocumentBlocks(const QString &documentId)
{
    QList<DocBlockRecord> list;
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        return list;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, document_id, order_index, block_type, content_text, formatting_json, version_updated, updated_by, updated_at "
        "FROM doc_blocks WHERE document_id = :doc ORDER BY order_index ASC"));
    q.bindValue(QStringLiteral(":doc"), documentId);

    if (q.exec()) {
        while (q.next()) {
            DocBlockRecord b;
            b.id = q.value(0).toString();
            b.documentId = q.value(1).toString();
            b.orderIndex = q.value(2).toDouble();
            b.blockType = q.value(3).toString();
            b.contentText = q.value(4).toString();
            b.formattingJson = q.value(5).toString();
            b.versionUpdated = q.value(6).toInt();
            b.updatedBy = q.value(7).toString();
            b.updatedAt = q.value(8).toString();
            list.append(b);
        }
    }
    return list;
}

bool DatabaseManager::saveOfficialDocumentBlocks(const QString &documentId, const QString &editorUserId,
                                                 const QList<DocBlockRecord> &blocks, int &newVersionOut,
                                                 QString &errorOut)
{
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        errorOut = tr("No active database connection.");
        return false;
    }

    db.transaction();

    int currentVer = 1;
    QSqlQuery verQ(db);
    verQ.prepare(QStringLiteral("SELECT current_version FROM documents WHERE id = :doc"));
    verQ.bindValue(QStringLiteral(":doc"), documentId);
    if (verQ.exec() && verQ.next()) {
        currentVer = verQ.value(0).toInt();
    }
    const int nextVer = currentVer + 1;
    const QString now = utcNowIso();

    // Load previous blocks map for forensic audit diff
    QMap<QString, QString> previousTextByBlock;
    for (const DocBlockRecord &oldB : loadDocumentBlocks(documentId)) {
        previousTextByBlock.insert(oldB.id, oldB.contentText);
    }

    QSqlQuery delQ(db);
    delQ.prepare(QStringLiteral("DELETE FROM doc_blocks WHERE document_id = :doc"));
    delQ.bindValue(QStringLiteral(":doc"), documentId);
    if (!delQ.exec()) {
        errorOut = delQ.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery insBlock(db);
    insBlock.prepare(QStringLiteral(
        "INSERT INTO doc_blocks (id, document_id, order_index, block_type, content_text, formatting_json, version_updated, updated_by, updated_at) "
        "VALUES (:id, :doc, :ord, :type, :txt, :fmt, :ver, :by, :at)"));

    QSqlQuery insAudit(db);
    insAudit.prepare(QStringLiteral(
        "INSERT INTO audit_history (id, document_id, element_type, element_id, version_num, changed_by, old_value, new_value, changed_at) "
        "VALUES (:id, :doc, 'BLOCK', :elId, :ver, :by, :oldV, :newV, :at)"));

    for (const DocBlockRecord &b : blocks) {
        const QString blockId = b.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : b.id;

        insBlock.bindValue(QStringLiteral(":id"), blockId);
        insBlock.bindValue(QStringLiteral(":doc"), documentId);
        insBlock.bindValue(QStringLiteral(":ord"), b.orderIndex);
        insBlock.bindValue(QStringLiteral(":type"), b.blockType.isEmpty() ? QStringLiteral("PARAGRAPH") : b.blockType);
        insBlock.bindValue(QStringLiteral(":txt"), b.contentText);
        insBlock.bindValue(QStringLiteral(":fmt"), b.formattingJson);
        insBlock.bindValue(QStringLiteral(":ver"), nextVer);
        insBlock.bindValue(QStringLiteral(":by"), editorUserId);
        insBlock.bindValue(QStringLiteral(":at"), now);

        if (!insBlock.exec()) {
            errorOut = insBlock.lastError().text();
            db.rollback();
            return false;
        }

        const QString oldText = previousTextByBlock.value(blockId);
        if (oldText != b.contentText) {
            insAudit.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
            insAudit.bindValue(QStringLiteral(":doc"), documentId);
            insAudit.bindValue(QStringLiteral(":elId"), blockId);
            insAudit.bindValue(QStringLiteral(":ver"), nextVer);
            insAudit.bindValue(QStringLiteral(":by"), editorUserId);
            insAudit.bindValue(QStringLiteral(":oldV"), oldText);
            insAudit.bindValue(QStringLiteral(":newV"), b.contentText);
            insAudit.bindValue(QStringLiteral(":at"), now);
            insAudit.exec();
        }
    }

    QSqlQuery updDoc(db);
    updDoc.prepare(QStringLiteral("UPDATE documents SET current_version = :ver, updated_at = :at WHERE id = :doc"));
    updDoc.bindValue(QStringLiteral(":ver"), nextVer);
    updDoc.bindValue(QStringLiteral(":at"), now);
    updDoc.bindValue(QStringLiteral(":doc"), documentId);
    if (!updDoc.exec()) {
        errorOut = updDoc.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        errorOut = db.lastError().text();
        return false;
    }

    newVersionOut = nextVer;
    return true;
}

QList<BlockAuditRecord> DatabaseManager::loadBlockAuditHistory(const QString &blockId)
{
    QList<BlockAuditRecord> list;
    QSqlDatabase db = activeDb();
    if (!db.isOpen() || blockId.isEmpty()) {
        return list;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT a.version_num, COALESCE(u.full_name, u.username), a.old_value, a.new_value, a.changed_at "
        "FROM audit_history a LEFT JOIN users u ON a.changed_by = u.id "
        "WHERE a.element_id = :elId ORDER BY a.version_num DESC"));
    q.bindValue(QStringLiteral(":elId"), blockId);

    if (q.exec()) {
        while (q.next()) {
            BlockAuditRecord rec;
            rec.versionNum = q.value(0).toInt();
            rec.changedByName = q.value(1).toString();
            rec.oldValue = q.value(2).toString();
            rec.newValue = q.value(3).toString();
            rec.changedAt = q.value(4).toString();
            list.append(rec);
        }
    }
    return list;
}

bool DatabaseManager::saveOrSubmitWriterProposal(const QString &documentId, const QString &authorId,
                                                 int baseVersion, const QString &proposalTitle,
                                                 const QList<DocBlockRecord> &proposedBlocks,
                                                 bool submitForReview, QString &errorOut)
{
    QSqlDatabase db = activeDb();
    if (!db.isOpen()) {
        errorOut = tr("No active database connection.");
        return false;
    }

    db.transaction();
    const QString now = utcNowIso();

    // Reuse existing DRAFT proposal by this author for this document if one exists
    QString proposalId;
    QSqlQuery findDraft(db);
    findDraft.prepare(QStringLiteral(
        "SELECT id FROM change_proposals WHERE document_id = :doc AND author_id = :author AND status = 'DRAFT' LIMIT 1"));
    findDraft.bindValue(QStringLiteral(":doc"), documentId);
    findDraft.bindValue(QStringLiteral(":author"), authorId);
    if (findDraft.exec() && findDraft.next()) {
        proposalId = findDraft.value(0).toString();
    }

    const QString targetStatus = submitForReview ? QStringLiteral("PENDING_REVIEW") : QStringLiteral("DRAFT");

    if (proposalId.isEmpty()) {
        proposalId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSqlQuery insProp(db);
        insProp.prepare(QStringLiteral(
            "INSERT INTO change_proposals (id, document_id, author_id, base_version, title, status, created_at, submitted_at) "
            "VALUES (:id, :doc, :author, :ver, :title, :status, :created, :sub)"));
        insProp.bindValue(QStringLiteral(":id"), proposalId);
        insProp.bindValue(QStringLiteral(":doc"), documentId);
        insProp.bindValue(QStringLiteral(":author"), authorId);
        insProp.bindValue(QStringLiteral(":ver"), baseVersion);
        insProp.bindValue(QStringLiteral(":title"), proposalTitle);
        insProp.bindValue(QStringLiteral(":status"), targetStatus);
        insProp.bindValue(QStringLiteral(":created"), now);
        insProp.bindValue(QStringLiteral(":sub"), submitForReview ? QVariant(now) : QVariant());
        if (!insProp.exec()) {
            errorOut = insProp.lastError().text();
            db.rollback();
            return false;
        }
    } else {
        QSqlQuery updProp(db);
        updProp.prepare(QStringLiteral(
            "UPDATE change_proposals SET title = :title, status = :status, submitted_at = :sub WHERE id = :id"));
        updProp.bindValue(QStringLiteral(":title"), proposalTitle);
        updProp.bindValue(QStringLiteral(":status"), targetStatus);
        updProp.bindValue(QStringLiteral(":sub"), submitForReview ? QVariant(now) : QVariant());
        updProp.bindValue(QStringLiteral(":id"), proposalId);
        updProp.exec();

        QSqlQuery clearItems(db);
        clearItems.prepare(QStringLiteral("DELETE FROM proposal_items WHERE proposal_id = :pid"));
        clearItems.bindValue(QStringLiteral(":pid"), proposalId);
        clearItems.exec();
    }

    QMap<QString, DocBlockRecord> officialById;
    for (const DocBlockRecord &ob : loadDocumentBlocks(documentId)) {
        officialById.insert(ob.id, ob);
    }

    QSqlQuery insItem(db);
    insItem.prepare(QStringLiteral(
        "INSERT INTO proposal_items (id, proposal_id, target_type, target_id, operation, old_payload_json, new_payload_json, review_status, updated_at) "
        "VALUES (:id, :pid, 'BLOCK', :tid, :op, :oldJ, :newJ, 'PENDING', :at)"));

    for (const DocBlockRecord &pb : proposedBlocks) {
        const bool existsInMain = officialById.contains(pb.id);
        const QString op = existsInMain ? QStringLiteral("UPDATE") : QStringLiteral("INSERT");

        if (existsInMain && officialById.value(pb.id).contentText == pb.contentText) {
            continue; // Unchanged paragraph
        }

        insItem.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
        insItem.bindValue(QStringLiteral(":pid"), proposalId);
        insItem.bindValue(QStringLiteral(":tid"), pb.id);
        insItem.bindValue(QStringLiteral(":op"), op);
        insItem.bindValue(QStringLiteral(":oldJ"), existsInMain ? officialById.value(pb.id).contentText : QString());
        insItem.bindValue(QStringLiteral(":newJ"), pb.contentText);
        insItem.bindValue(QStringLiteral(":at"), now);
        insItem.exec();
    }

    // Notify document owner when submitted for review
    if (submitForReview) {
        QSqlQuery ownerQ(db);
        ownerQ.prepare(QStringLiteral("SELECT owner_id, title FROM documents WHERE id = :doc"));
        ownerQ.bindValue(QStringLiteral(":doc"), documentId);
        if (ownerQ.exec() && ownerQ.next()) {
            const QString ownerId = ownerQ.value(0).toString();
            const QString docTitle = ownerQ.value(1).toString();

            QSqlQuery notifQ(db);
            notifQ.prepare(QStringLiteral(
                "INSERT INTO notifications (id, recipient_id, sender_id, document_id, proposal_id, notif_type, message, is_read, created_at) "
                "VALUES (:id, :rec, :snd, :doc, :pid, 'PROPOSAL_SUBMITTED', :msg, 0, :at)"));
            notifQ.bindValue(QStringLiteral(":id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
            notifQ.bindValue(QStringLiteral(":rec"), ownerId);
            notifQ.bindValue(QStringLiteral(":snd"), authorId);
            notifQ.bindValue(QStringLiteral(":doc"), documentId);
            notifQ.bindValue(QStringLiteral(":pid"), proposalId);
            notifQ.bindValue(QStringLiteral(":msg"), tr("New change proposal submitted for '%1'").arg(docTitle));
            notifQ.bindValue(QStringLiteral(":at"), now);
            notifQ.exec();
        }
    }

    if (!db.commit()) {
        errorOut = db.lastError().text();
        return false;
    }
    return true;
}
