#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QLibraryInfo>
#include <QSettings>
#include <QDir>
#include <QDebug>

LanguageManager &LanguageManager::instance()
{
    static LanguageManager s_instance;
    return s_instance;
}

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
    , m_currentLanguage(QStringLiteral("en"))
{
}

void LanguageManager::initialize()
{
    QSettings settings(QStringLiteral("CollabOffice"), QStringLiteral("ClientPreferences"));
    QString savedLang = settings.value(QStringLiteral("ui/language")).toString();

    if (savedLang.isEmpty()) {
        const QString sysLang = QLocale::system().name().left(2).toLower();
        savedLang = (sysLang == QStringLiteral("es")) ? QStringLiteral("es") : QStringLiteral("en");
    }

    setLanguage(savedLang);
}

bool LanguageManager::setLanguage(const QString &langCode)
{
    const QString normalized = normalizeLanguageCode(langCode);
    if (normalized == m_currentLanguage && !m_appTranslator.isEmpty()) {
        return true;
    }

    qApp->removeTranslator(&m_appTranslator);
    qApp->removeTranslator(&m_qtBaseTranslator);

    const bool loaded = loadTranslatorForCode(normalized);
    m_currentLanguage = normalized;

    QSettings settings(QStringLiteral("CollabOffice"), QStringLiteral("ClientPreferences"));
    settings.setValue(QStringLiteral("ui/language"), m_currentLanguage);

    emit languageChanged(m_currentLanguage);
    return loaded;
}

QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

QStringList LanguageManager::supportedLanguages() const
{
    return { QStringLiteral("es"), QStringLiteral("en") };
}

QString LanguageManager::languageDisplayName(const QString &langCode) const
{
    const QString normalized = normalizeLanguageCode(langCode);
    if (normalized == QStringLiteral("es")) {
        return QStringLiteral("Español");
    }
    return QStringLiteral("English");
}

QString LanguageManager::normalizeLanguageCode(const QString &code) const
{
    const QString clean = code.trimmed().left(2).toLower();
    if (clean == QStringLiteral("es")) {
        return QStringLiteral("es");
    }
    return QStringLiteral("en");
}

bool LanguageManager::loadTranslatorForCode(const QString &langCode)
{
    const QString qmBaseName = QStringLiteral("lang_%1").arg(langCode);

    // Search paths: embedded Qt resources first, then filesystem deployment folders
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList searchPaths = {
        QStringLiteral(":/translations"),
        appDir + QStringLiteral("/translations"),
        appDir,
        appDir + QStringLiteral("/../translations"),
        QDir::currentPath() + QStringLiteral("/translations")
    };

    bool appLoaded = false;
    for (const QString &path : searchPaths) {
        if (m_appTranslator.load(qmBaseName, path)) {
            qApp->installTranslator(&m_appTranslator);
            appLoaded = true;
            break;
        }
    }

    // Load standard Qt dialog button translations (OK, Cancel, Yes, No, Print, etc.)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    const QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif

    if (m_qtBaseTranslator.load(QStringLiteral("qtbase_%1").arg(langCode), qtTranslationsPath)) {
        qApp->installTranslator(&m_qtBaseTranslator);
    }

    if (!appLoaded) {
        qWarning() << "[LanguageManager] Translation package not found for:" << qmBaseName;
    }

    return appLoaded;
}
