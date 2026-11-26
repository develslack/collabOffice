#ifndef WRITERMODULE_H
#define WRITERMODULE_H

#include <QObject>
#include <QList>
#include "IWorkspaceModule.h"
#include "DatabaseManager.h"
#include "StyleGalleryWidget.h"

class QTextEdit;
class QFontComboBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QScrollArea;
class PageRulerWidget;
class WriterInsertTabHandler;
class WriterDrawTabHandler;

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

    // Font actions
    void onGrowFont();
    void onShrinkFont();
    void onClearFormatting();
    void onToggleBold();
    void onToggleItalic();
    void onToggleUnderline();
    void onToggleStrikeOut();
    void onToggleSubscript();
    void onToggleSuperscript();
    void onHighlightColorClicked();
    void onTextColorClicked();
    void onFontFamilyChanged(const QFont &font);
    void onFontSizeChanged(const QString &sizeText);

    // Paragraph actions
    void onAlignLeft();
    void onAlignCenter();
    void onAlignRight();
    void onAlignJustify();
    void onInsertBulletList();
    void onInsertNumberedList();
    void onIncreaseIndent();
    void onDecreaseIndent();
    void onToggleLineSpacing();
    void onToggleNonPrintingMarks(bool checked);
    void onParagraphShadingClicked();

    // Styles gallery
    void onStylePresetSelected(StyleGalleryWidget::StylePreset preset);

    // Canvas & metrics
    void onZoomSliderChanged(int percent);
    void onCursorPositionChanged();
    void onDocumentContentsChanged();

private:
    void buildUi();
    void buildRibbonWidgets();
    void buildInspectorDock();
    void loadBlocksIntoEditor();
    void updateWordAndPageMetrics();
    QList<DocBlockRecord> extractBlocksFromEditor() const;
    bool isOwnerOrCoAdmin() const;
    bool isReadOnlyMode() const;

    DocumentSummary m_doc;
    UserSession m_session;
    bool m_isDirty;
    bool m_isLoading;
    int m_currentZoomPercent;
    bool m_doubleSpacingActive;

    // Main page canvas & rulers
    QWidget *m_rootWidget;
    QScrollArea *m_pageScrollArea;
    PageRulerWidget *m_topRuler;
    PageRulerWidget *m_leftRuler;
    QTextEdit *m_editor;

    // Bottom status strip inside Writer
    QLabel *m_lblPageCount;
    QLabel *m_lblWordCount;
    QLabel *m_lblZoomPercent;
    QSlider *m_zoomSlider;

    // Rich Ribbon group widgets
    QWidget *m_fontGroupWidget;
    QFontComboBox *m_fontCombo;
    QComboBox *m_fontSizeCombo;
    QWidget *m_paragraphGroupWidget;
    StyleGalleryWidget *m_styleGallery;

    // Contextual Inspector Dock
    QWidget *m_dockWidget;
    QLabel *m_lblDocMeta;
    QLabel *m_lblModeBanner;
    QPushButton *m_btnSubmitProposal;
    QListWidget *m_listParagraphAudit;

    // Actions
    QAction *m_actSave;
    QAction *m_actSubmitProposal;
    QAction *m_actExportPdf;
    QAction *m_actPrint;
    QAction *m_actUndo;
    QAction *m_actRedo;

    QAction *m_actPaste;
    QAction *m_actCut;
    QAction *m_actCopy;

    QAction *m_actGrowFont;
    QAction *m_actShrinkFont;
    QAction *m_actClearFormat;
    QAction *m_actBold;
    QAction *m_actItalic;
    QAction *m_actUnderline;
    QAction *m_actStrikeOut;
    QAction *m_actSubscript;
    QAction *m_actSuperscript;
    QAction *m_actHighlightColor;
    QAction *m_actTextColor;

    QAction *m_actBulletList;
    QAction *m_actNumberList;
    QAction *m_actDecreaseIndent;
    QAction *m_actIncreaseIndent;
    QAction *m_actLineSpacing;
    QAction *m_actShowMarks;
    QAction *m_actAlignLeft;
    QAction *m_actAlignCenter;
    QAction *m_actAlignRight;
    QAction *m_actAlignJustify;
    QAction *m_actParagraphShading;

    QList<QString> m_blockIdsByIndex;
    WriterInsertTabHandler *m_insertHandler;

    WriterDrawTabHandler *m_drawHandler;
};

#endif // WRITERMODULE_H
