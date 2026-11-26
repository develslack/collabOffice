TEMPLATE = app
TARGET   = CollabOffice
VERSION  = 0.1.0

QT += core gui sql network printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 warn_on
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050D00
DEFINES += APP_NAME=\"\\\"$$TARGET\\\"\"
DEFINES += APP_VERSION=\"\\\"$$VERSION\\\"\"

isEmpty(PREFIX) {
    unix:!macx: PREFIX = /opt/$$TARGET
    win32:      PREFIX = "C:/Program Files/$$TARGET"
}

isEmpty(DESTDIR): DESTDIR = $$PWD/build
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR     = $$DESTDIR/.moc
RCC_DIR     = $$DESTDIR/.rcc
UI_DIR      = $$DESTDIR/.ui

unix:!macx {
    QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN/../libs\'
}

win32 {
    CONFIG(release, debug|release): CONFIG -= console
    exists($$PWD/resources/icons/CollabOffice.ico) {
        RC_ICONS = $$PWD/resources/icons/CollabOffice.ico
    }
}

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/core \
    $$PWD/src/core/i18n \
    $$PWD/src/core/db \
    $$PWD/src/core/security \
    $$PWD/src/core/mail \
    $$PWD/src/core/sync \
    $$PWD/src/modules \
    $$PWD/src/modules/admin \
    $$PWD/src/modules/browser \
    $$PWD/src/modules/writer \
    $$PWD/src/modules/calc \
    $$PWD/src/modules/migration \
    $$PWD/src/ui \
    $$PWD/src/ui/dialogs \
    $$PWD/src/ui/widgets

HEADERS += \
    src/core/QtCompat.h \
    src/core/IWorkspaceModule.h \
    src/core/i18n/LanguageManager.h \
    src/core/db/BootstrapManager.h \
    src/core/db/DatabaseManager.h \
    src/core/security/PasswordUtils.h \
    src/core/mail/SmtpClient.h \
    src/modules/admin/AdminModule.h \
    src/modules/browser/DocumentBrowserModule.h \
    src/modules/writer/WriterModule.h \
    src/modules/writer/PageRulerWidget.h \
    src/modules/writer/StyleGalleryWidget.h \
    src/modules/writer/WriterInsertTabHandler.h \
    src/modules/writer/PageInkOverlayWidget.h \
    src/modules/writer/WriterDrawTabHandler.h \
    src/ui/MainWindow.h \
    src/ui/dialogs/SetupWizardDialog.h \
    src/ui/dialogs/LoginDialog.h \
    src/ui/dialogs/ChangePasswordDialog.h

SOURCES += \
    src/core/i18n/LanguageManager.cpp \
    src/core/db/BootstrapManager.cpp \
    src/core/db/DatabaseManager.cpp \
    src/core/security/PasswordUtils.cpp \
    src/core/mail/SmtpClient.cpp \
    src/modules/admin/AdminModule.cpp \
    src/modules/browser/DocumentBrowserModule.cpp \
    src/modules/writer/WriterModule.cpp \
    src/modules/writer/PageRulerWidget.cpp \
    src/modules/writer/StyleGalleryWidget.cpp \
    src/modules/writer/WriterInsertTabHandler.cpp \
    src/modules/writer/PageInkOverlayWidget.cpp \
    src/modules/writer/WriterDrawTabHandler.cpp \
    src/ui/MainWindow.cpp \
    src/ui/dialogs/SetupWizardDialog.cpp \
    src/ui/dialogs/LoginDialog.cpp \
    src/ui/dialogs/ChangePasswordDialog.cpp \
    src/main.cpp

TRANSLATIONS += \
    translations/lang_es.ts \
    translations/lang_en.ts

greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 11) {
    CONFIG += lrelease
}

RESOURCES += resources/resources.qrc

target.path = $$PREFIX/bin
INSTALLS += target
