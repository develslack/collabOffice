#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QList>
#include "BootstrapManager.h"
#include "SmtpClient.h"

struct UserSession {
    QString userId;
    QString username;
    QString fullName;
    QString email;
    QString systemRole;        // "ADMIN" or "USER"
    bool mustChangePassword = false;
    QString uiMode;            // "RIBBON" or "CLASSIC"
    QString shortcutProfile;   // "ES" or "EN"
    QString theme;             // "LIGHT" or "DARK"

    bool isValid() const { return !userId.isEmpty(); }
    bool isAdmin() const { return systemRole == QStringLiteral("ADMIN"); }
};

struct UserRecord {
    QString id;
    QString username;
    QString fullName;
    QString email;
    QString systemRole;
    bool mustChangePassword = false;
    bool isActive = true;
    QString createdAt;
};

struct HostRecord {
    QString id;
    int hostNumber = 0;
    QString hostLabel;
    QString machineName;
    QString registeredAt;
};

struct GroupRecord {
    QString id;
    QString name;
    QString description;
    int memberCount = 0;
};

struct DocumentSummary {
    QString id;
    QString title;
    QString docType;           // "TEXT_DOC" or "SPREADSHEET"
    QString ownerId;
    QString ownerName;
    int currentVersion = 1;
    QString effectiveAccess;   // "OWNER", "ADMIN_VIEW", "CO_ADMIN", "PROPOSE_EDIT", "READ"
    QString updatedAt;
};

struct DocBlockRecord {
    QString id;
    QString documentId;
    double orderIndex = 1000.0;
    QString blockType;         // "PARAGRAPH", "HEADING"
    QString contentText;
    QString formattingJson;
    int versionUpdated = 1;
    QString updatedBy;
    QString updatedAt;
};

struct BlockAuditRecord {
    int versionNum = 1;
    QString changedByName;
    QString oldValue;
    QString newValue;
    QString changedAt;
};

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager &instance();

    // Provisioning & connectivity
    bool testMariaDbConnection(const QString &host, int port, const QString &user, const QString &password, QString &errorOut);
    bool provisionDatabaseAndSchema(const QString &host, int port, const QString &dbName,
                                    const QString &user, const QString &password, QString &errorOut);
    bool connectFromBootstrap(const BootstrapConfig &config, QString &errorOut);
    bool isRemoteOnline() const;

    // Sequential Host Registration (PC-001, PC-002, ...)
    bool registerSequentialHost(const QString &machineName, int &hostNumberOut,
                                QString &hostLabelOut, QString &hostIdOut, QString &errorOut);
    void updateHostHeartbeat(const QString &hostId);

    // User Provisioning & Authentication
    bool administratorExists(bool &existsOut, QString &errorOut);
    bool createSoleAdministrator(const QString &fullName, const QString &username,
                                 const QString &email, const QString &plainPassword,
                                 const QString &hostId, QString &errorOut);
    bool createStandardUser(const QString &fullName, const QString &username,
                            const QString &email, const QString &hostId,
                            QString &generatedPasswordOut, QString &errorOut);

    bool authenticateUser(const QString &usernameOrEmail, const QString &plainPassword,
                          UserSession &sessionOut, QString &errorOut);
    bool changeUserPassword(const QString &userId, const QString &newPlainPassword, QString &errorOut);
    bool resetPasswordForEmail(const QString &email, QString &fullNameOut, QString &usernameOut,
                               QString &tempPasswordOut, QString &errorOut);

    // Corporate Email Settings (stored in DB, shared across all hosts)
    MailServerConfig loadMailServerConfig();
    bool saveMailServerConfig(const MailServerConfig &config, QString &errorOut);

    // Admin queries
    QList<UserRecord> listAllUsers();
    QList<HostRecord> listAllHosts();
    QList<GroupRecord> listAllGroups();
    bool createGroup(const QString &name, const QString &description, const QString &createdByUserId, QString &errorOut);

    // Document Repository & Permission-Scoped Browser
    QList<DocumentSummary> listAccessibleDocuments(const UserSession &session);
    bool createDocument(const QString &title, const QString &docType, const QString &ownerId,
                        QString &newDocIdOut, QString &errorOut);
    bool grantDocumentPermission(const QString &documentId, const QString &targetUserId,
                                 const QString &targetGroupId, const QString &accessLevel,
                                 const QString &grantedByUserId, QString &errorOut);

    // Writer Blocks (Official Branch)
    QList<DocBlockRecord> loadDocumentBlocks(const QString &documentId);
    bool saveOfficialDocumentBlocks(const QString &documentId, const QString &editorUserId,
                                    const QList<DocBlockRecord> &blocks, int &newVersionOut,
                                    QString &errorOut);
    QList<BlockAuditRecord> loadBlockAuditHistory(const QString &blockId);

    // Collaborative Virtual Drafts / Proposals
    bool saveOrSubmitWriterProposal(const QString &documentId, const QString &authorId,
                                    int baseVersion, const QString &proposalTitle,
                                    const QList<DocBlockRecord> &proposedBlocks,
                                    bool submitForReview, QString &errorOut);

signals:
    void connectionStatusChanged(bool remoteOnline);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool initializeLocalSqliteMirror(QString &errorOut);
    bool executeSqlScript(QSqlDatabase &db, const QString &resourcePath, QString &errorOut);
    QSqlDatabase activeDb() const;

    bool m_remoteOnline;
    QString m_remoteConnName;
    QString m_localConnName;
};

#endif // DATABASEMANAGER_H
