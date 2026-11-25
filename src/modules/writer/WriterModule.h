#ifndef WRITERMODULE_H
#define WRITERMODULE_H

#include <QObject>
#include <QList>
#include "IWorkspaceModule.h"
#include "DatabaseManager.h"

class QTextEdit;
class QFontComboBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;

class WriterModule : public QObject, public IWorkspaceModule
{
    Q_OBJECT

public:
    explicit WriterModule(const DocumentSummary &docSummary,
                          const UserSession &session,
                          QObject *parent = nullptr);
    ~WriterModule() override = default;

    QString moduleId() const override;
    QString displayTitle() const override;
    QIcon moduleIcon() const override;
    QWidget *centralWidget() override;
    QWidget *contextualDockWidget() override;
    QList<CommandGroupSpec> commandGroups() const override;
    QString statusSummary() const override;

    bool hasUnsavedChanges() const override;
    bool saveChanges() override;

    void retranslateUi() override;
    void applyShortcutProfile(const QString &profileCode) override;

signals:
    void documentSaved(const QString &documentId);

private slots:
    void onSaveTriggered();
    void onSubmitProposalTriggered();
    void onExportPdfTriggered();
    void onPrintTriggered();

    void onToggleBold();
    void onToggleItalic();
    void onToggleUnderline();
    void onAlignLeft();
    void onAlignCenter();
    void onAlignRight();
    void onAlignJustify();
    void onInsertBulletList();
    void onInsertNumberedList();
    void onTextColorClicked();
    void onFontFamilyChanged(const QFont &font);
    void onFontSizeChanged(const QString &sizeText);

    void onCursorPositionChanged();
    void onDocumentContentsChanged();

private:
    void buildUi();
    void buildInspectorDock();
    void loadBlocksIntoEditor();
    QList<DocBlockRecord> extractBlocksFromEditor() const;
    bool isOwnerOrCoAdmin() const;
    bool isReadOnlyMode() const;

    DocumentSummary m_doc;
    UserSession m_session;
    bool m_isDirty;
    bool m_isLoading;

    QWidget *m_rootWidget;
    QTextEdit *m_editor;
    QFontComboBox *m_fontCombo;
    QComboBox *m_fontSizeCombo;

    // Contextual Inspector Dock
    QWidget *m_dockWidget;
    QLabel *m_lblDocMeta;
    QLabel *m_lblModeBanner;
    QPushButton *m_btnSubmitProposal;
    QListWidget *m_listParagraphAudit;

    // Actions (used by MainWindow Ribbon / Classic menus)
    QAction *m_actSave;
    QAction *m_actSubmitProposal;
    QAction *m_actExportPdf;
    QAction *m_actPrint;
    QAction *m_actUndo;
    QAction *m_actRedo;
    QAction *m_actBold;
    QAction *m_actItalic;
    QAction *m_actUnderline;
    QAction *m_actTextColor;
    QAction *m_actAlignLeft;
    QAction *m_actAlignCenter;
    QAction *m_actAlignRight;
    QAction *m_actAlignJustify;
    QAction *m_actBulletList;
    QAction *m_actNumberList;

    QList<QString> m_blockIdsByIndex;
};

#endif // WRITERMODULE_H
