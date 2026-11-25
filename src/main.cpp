#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QList>
#include "MainWindow.h"
#include "LanguageManager.h"
#include "BootstrapManager.h"
#include "DatabaseManager.h"
#include "SetupWizardDialog.h"
#include "LoginDialog.h"
#include "ChangePasswordDialog.h"
#include "AdminModule.h"
#include "DocumentBrowserModule.h"
#include "WriterModule.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(APP_NAME));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName(QStringLiteral("CollabOffice"));

    LanguageManager::instance().initialize();

    BootstrapConfig bootstrapCfg;
    if (!BootstrapManager::isBootstrapped()) {
        SetupWizardDialog wizard;
        if (wizard.exec() != QDialog::Accepted) {
            return 0;
        }
        bootstrapCfg = wizard.completedConfig();
    } else {
        bootstrapCfg = BootstrapManager::loadConfig();
        QString dbErr;
        if (!DatabaseManager::instance().connectFromBootstrap(bootstrapCfg, dbErr)) {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Remote Database Offline"),
                                 QObject::tr("Could not connect to remote MariaDB server (%1).\n"
                                             "CollabOffice will continue using the local SQLite cache until connection is restored.")
                                     .arg(dbErr));
        }
    }

    // Session loop: allows clean Sign Out -> Sign In without restarting the app
    while (true) {
        UserSession activeSession;
        bool authenticated = false;

        while (!authenticated) {
            LoginDialog loginDlg(bootstrapCfg.hostLabel);
            if (loginDlg.exec() != QDialog::Accepted) {
                return 0;
            }

            UserSession candidate = loginDlg.authenticatedSession();
            if (candidate.mustChangePassword) {
                ChangePasswordDialog pwdDlg(candidate.userId, candidate.username);
                if (pwdDlg.exec() == QDialog::Accepted) {
                    continue;
                }
                return 0;
            }

            activeSession = candidate;
            authenticated = true;
        }

        MainWindow mainWindow;
        mainWindow.setCurrentUserSession(activeSession, bootstrapCfg.hostLabel);
        mainWindow.setConnectionState(DatabaseManager::instance().isRemoteOnline()
                                          ? MainWindow::ConnectionState::MariaDbOnline
                                          : MainWindow::ConnectionState::SqliteOfflineFallback);

        bool reloginRequested = false;
        QList<IWorkspaceModule *> ownedModules;

        auto openDocumentInModule = [&](const DocumentSummary &doc) {
            if (doc.docType == QStringLiteral("TEXT_DOC")) {
                WriterModule *writer = new WriterModule(doc, activeSession, &mainWindow);
                ownedModules.append(writer);
                mainWindow.registerModule(writer, true);
            } else {
                QMessageBox::information(&mainWindow,
                                         QObject::tr("Spreadsheet Module"),
                                         QObject::tr("Spreadsheet editor module will be mounted here in the next step."));
            }
        };

        auto createNewTextDocFlow = [&]() {
            bool ok = false;
            const QString title = QInputDialog::getText(
                &mainWindow,
                QObject::tr("New Text Document"),
                QObject::tr("Enter document title:"),
                QLineEdit::Normal,
                QString(),
                &ok).trimmed();

            if (!ok || title.isEmpty()) {
                return;
            }

            QString newDocId;
            QString err;
            if (!DatabaseManager::instance().createDocument(title, QStringLiteral("TEXT_DOC"), activeSession.userId, newDocId, err)) {
                QMessageBox::critical(&mainWindow, QObject::tr("Error"), err);
                return;
            }

            DocumentSummary summary;
            summary.id = newDocId;
            summary.title = title;
            summary.docType = QStringLiteral("TEXT_DOC");
            summary.ownerId = activeSession.userId;
            summary.ownerName = activeSession.fullName;
            summary.currentVersion = 1;
            summary.effectiveAccess = QStringLiteral("OWNER");

            openDocumentInModule(summary);
        };

        // Mount Document Browser by default so user immediately sees their scoped documents
        DocumentBrowserModule *browserModule = new DocumentBrowserModule(activeSession, &mainWindow);
        ownedModules.append(browserModule);
        mainWindow.registerModule(browserModule, true);

        QObject::connect(&mainWindow, &MainWindow::openDocumentBrowserRequested, [&]() {
            browserModule->refreshDocumentList();
            mainWindow.registerModule(browserModule, true);
        });
        QObject::connect(&mainWindow, &MainWindow::newTextDocumentRequested, createNewTextDocFlow);
        QObject::connect(browserModule, &DocumentBrowserModule::createNewTextDocumentRequested, createNewTextDocFlow);
        QObject::connect(browserModule, &DocumentBrowserModule::openDocumentRequested, openDocumentInModule);

        if (activeSession.isAdmin()) {
            AdminModule *adminModule = new AdminModule(activeSession, &mainWindow);
            ownedModules.append(adminModule);
            QObject::connect(&mainWindow, &MainWindow::openUserAdminRequested, [&]() {
                mainWindow.registerModule(adminModule, true);
            });
        }

        QObject::connect(&mainWindow, &MainWindow::logoutRequested, [&]() {
            reloginRequested = true;
            mainWindow.close();
        });

        mainWindow.show();
        const int exitCode = app.exec();

        if (!reloginRequested) {
            return exitCode;
        }
    }
}
