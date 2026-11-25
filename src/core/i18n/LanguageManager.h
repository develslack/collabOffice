#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QString>
#include <QStringList>

class LanguageManager : public QObject
{
    Q_OBJECT

public:
    static LanguageManager &instance();

    // Initializes language from saved QSettings or system locale ("es" / "en")
    void initialize();

    // Switches active UI language ("es" -> lang_es.qm, "en" -> lang_en.qm)
    bool setLanguage(const QString &langCode);

    QString currentLanguage() const;
    QStringList supportedLanguages() const;
    QString languageDisplayName(const QString &langCode) const;

signals:
    void languageChanged(const QString &newLangCode);

private:
    explicit LanguageManager(QObject *parent = nullptr);
    ~LanguageManager() override = default;

    LanguageManager(const LanguageManager &) = delete;
    LanguageManager &operator=(const LanguageManager &) = delete;

    QString normalizeLanguageCode(const QString &code) const;
    bool loadTranslatorForCode(const QString &langCode);

    QTranslator m_appTranslator;
    QTranslator m_qtBaseTranslator;
    QString m_currentLanguage;
};

#endif // LANGUAGEMANAGER_H
