#include "WriterModule.h"
#include "PageRulerWidget.h"
#include "QtCompat.h"
#include "WriterInsertTabHandler.h"
#include "WriterDrawTabHandler.h"

#include <QTextFrame>
#include <QTextTable>
#include <QTextDocumentFragment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextListFormat>
#include <QTextOption>
#include <QFontComboBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QSlider>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QMessageBox>
#include <QUuid>
#include <QApplication>
#include <QStyle>

WriterModule::WriterModule(const DocumentSummary &docSummary,
                           const UserSession &session,
                           QObject *parent)
    : QObject(parent)
    , m_doc(docSummary)
    , m_session(session)
    , m_isDirty(false)
    , m_isLoading(false)
    , m_currentZoomPercent(100)
    , m_doubleSpacingActive(false)
    , m_rootWidget(nullptr)
    , m_pageScrollArea(nullptr)
    , m_topRuler(nullptr)
    , m_leftRuler(nullptr)
    , m_editor(nullptr)
    , m_lblPageCount(nullptr)
    , m_lblWordCount(nullptr)
    , m_lblZoomPercent(nullptr)
    , m_zoomSlider(nullptr)
    , m_fontGroupWidget(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeCombo(nullptr)
    , m_paragraphGroupWidget(nullptr)
    , m_styleGallery(nullptr)
    , m_dockWidget(nullptr)
    , m_lblDocMeta(nullptr)
    , m_lblModeBanner(nullptr)
    , m_btnSubmitProposal(nullptr)
    , m_listParagraphAudit(nullptr)
    , m_insertHandler(nullptr)
    , m_drawHandler(nullptr)
{
    buildUi();
    buildRibbonWidgets();
    buildInspectorDock();
    loadBlocksIntoEditor();
    updateWordAndPageMetrics();
}


QString WriterModule::moduleId() const
{
    return QStringLiteral("writer:%1").arg(m_doc.id);
}

QString WriterModule::displayTitle() const
{
    return m_isDirty ? QStringLiteral("* %1").arg(m_doc.title) : m_doc.title;
}

QIcon WriterModule::moduleIcon() const
{
    return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
}

QWidget *WriterModule::centralWidget()
{
    return m_rootWidget;
}

QWidget *WriterModule::contextualDockWidget()
{
    return m_dockWidget;
}

QList<CommandGroupSpec> WriterModule::commandGroups() const
{
    // Tab 1: File (Archivo)
    CommandGroupSpec fileTabGrp;
    fileTabGrp.tabTitle = tr("File");
    fileTabGrp.groupTitle = tr("Document & Export");
    fileTabGrp.actions = { m_actSave };
    if (!isOwnerOrCoAdmin() && !isReadOnlyMode()) {
        fileTabGrp.actions.append(m_actSubmitProposal);
    }
    fileTabGrp.actions << m_actExportPdf << m_actPrint << m_actUndo << m_actRedo;

    // Tab 2: Home (Inicio) - Matching ONLYOFFICE layout
    CommandGroupSpec clipGrp;
    clipGrp.tabTitle = tr("Home");
    clipGrp.groupTitle = tr("Clipboard");
    clipGrp.actions = { m_actPaste, m_actCut, m_actCopy };

    CommandGroupSpec fontGrp;
    fontGrp.tabTitle = tr("Home");
    fontGrp.groupTitle = tr("Font");
    fontGrp.actions = {
        m_actGrowFont, m_actShrinkFont, m_actClearFormat,
        m_actBold, m_actItalic, m_actUnderline, m_actStrikeOut,
        m_actSubscript, m_actSuperscript, m_actHighlightColor, m_actTextColor
    };
    fontGrp.customWidget = m_fontGroupWidget;

    CommandGroupSpec paraGrp;
    paraGrp.tabTitle = tr("Home");
    paraGrp.groupTitle = tr("Paragraph");
    paraGrp.actions = {
        m_actBulletList, m_actNumberList, m_actDecreaseIndent, m_actIncreaseIndent,
        m_actLineSpacing, m_actShowMarks,
        m_actAlignLeft, m_actAlignCenter, m_actAlignRight, m_actAlignJustify,
        m_actParagraphShading
    };
    paraGrp.customWidget = m_paragraphGroupWidget;

    CommandGroupSpec stylesGrp;
    stylesGrp.tabTitle = tr("Home");
    stylesGrp.groupTitle = tr("Styles");
    stylesGrp.customWidget = m_styleGallery;

    QList<CommandGroupSpec> groups = { fileTabGrp, clipGrp, fontGrp, paraGrp, stylesGrp };
    if (m_insertHandler) {
        groups.append(m_insertHandler->commandGroups());
    }
    if (m_drawHandler) {
        groups.append(m_drawHandler->commandGroups());
    }
    return groups;

}

QString WriterModule::statusSummary() const
{
    if (isReadOnlyMode()) {
        return tr("Read-Only Mode | Official Version v%1 | Owner: %2").arg(m_doc.currentVersion).arg(m_doc.ownerName);
    }
    if (isOwnerOrCoAdmin()) {
        return tr("Direct Official Editing | Version v%1 | Owner: %2").arg(m_doc.currentVersion).arg(m_doc.ownerName);
    }
    return tr("Virtual Contributor Draft (Base v%1) | Owner: %2").arg(m_doc.currentVersion).arg(m_doc.ownerName);
}

bool WriterModule::hasUnsavedChanges() const
{
    return m_isDirty && !isReadOnlyMode();
}

bool WriterModule::saveChanges()
{
    if (isReadOnlyMode()) {
        return true;
    }
    if (m_drawHandler && m_drawHandler->hasPendingInk()) {
        m_drawHandler->commitInkToDocument();
    }

    const QList<DocBlockRecord> blocks = extractBlocksFromEditor();
    QString err;

    if (isOwnerOrCoAdmin()) {
        int newVer = m_doc.currentVersion;
        if (!DatabaseManager::instance().saveOfficialDocumentBlocks(m_doc.id, m_session.userId, blocks, newVer, err)) {
            QMessageBox::critical(m_rootWidget, tr("Save Error"), err);
            return false;
        }
        m_doc.currentVersion = newVer;
    } else {
        const QString propTitle = tr("Draft update by %1").arg(m_session.fullName);
        if (!DatabaseManager::instance().saveOrSubmitWriterProposal(
                m_doc.id, m_session.userId, m_doc.currentVersion, propTitle, blocks, false, err)) {
            QMessageBox::critical(m_rootWidget, tr("Draft Save Error"), err);
            return false;
        }
    }

    m_isDirty = false;
    emit documentSaved(m_doc.id);
    return true;
}

void WriterModule::retranslateUi()
{
    m_actSave->setText(isOwnerOrCoAdmin() ? tr("Save") : tr("Save Draft"));
    m_actSubmitProposal->setText(tr("Send to Owner"));
    m_actExportPdf->setText(tr("Export PDF"));
    m_actPrint->setText(tr("Print"));
    m_actUndo->setText(tr("Undo"));
    m_actRedo->setText(tr("Redo"));

    m_actPaste->setText(tr("Paste"));
    m_actCut->setText(tr("Cut"));
    m_actCopy->setText(tr("Copy"));

    m_actGrowFont->setToolTip(tr("Increase Font Size"));
    m_actShrinkFont->setToolTip(tr("Decrease Font Size"));
    m_actClearFormat->setToolTip(tr("Clear Formatting"));
    m_actBold->setToolTip(tr("Bold"));
    m_actItalic->setToolTip(tr("Italic"));
    m_actUnderline->setToolTip(tr("Underline"));
    m_actStrikeOut->setToolTip(tr("Strikethrough"));
    m_actSubscript->setToolTip(tr("Subscript"));
    m_actSuperscript->setToolTip(tr("Superscript"));
    m_actHighlightColor->setToolTip(tr("Text Highlight Color"));
    m_actTextColor->setToolTip(tr("Font Color"));

    m_actBulletList->setToolTip(tr("Bulleted List"));
    m_actNumberList->setToolTip(tr("Numbered List"));
    m_actDecreaseIndent->setToolTip(tr("Decrease Indent"));
    m_actIncreaseIndent->setToolTip(tr("Increase Indent"));
    m_actLineSpacing->setToolTip(tr("Line Spacing"));
    m_actShowMarks->setToolTip(tr("Show/Hide Non-Printing Characters"));
    m_actAlignLeft->setToolTip(tr("Align Left"));
    m_actAlignCenter->setToolTip(tr("Align Center"));
    m_actAlignRight->setToolTip(tr("Align Right"));
    m_actAlignJustify->setToolTip(tr("Justify"));
    m_actParagraphShading->setToolTip(tr("Paragraph Shading"));

    if (m_styleGallery) {
        m_styleGallery->retranslateUi();
    }
    if (m_btnSubmitProposal) {
        m_btnSubmitProposal->setText(tr("Submit Changes to Owner for Review"));
    }
    if (m_insertHandler) {
        m_insertHandler->retranslateUi();
    }
    if (m_drawHandler) {
        m_drawHandler->retranslateUi();
    }

    updateWordAndPageMetrics();
}

void WriterModule::applyShortcutProfile(const QString &profileCode)
{
    const bool isSpanish = (profileCode == QStringLiteral("ES"));
    m_actSave->setShortcut(isSpanish ? QKeySequence(QStringLiteral("Ctrl+G")) : QKeySequence::Save);
    m_actBold->setShortcut(isSpanish ? QKeySequence(QStringLiteral("Ctrl+N")) : QKeySequence(QStringLiteral("Ctrl+B")));
    m_actItalic->setShortcut(isSpanish ? QKeySequence(QStringLiteral("Ctrl+K")) : QKeySequence(QStringLiteral("Ctrl+I")));
    m_actUnderline->setShortcut(isSpanish ? QKeySequence(QStringLiteral("Ctrl+S")) : QKeySequence(QStringLiteral("Ctrl+U")));
}

bool WriterModule::isOwnerOrCoAdmin() const
{
    return (m_doc.ownerId == m_session.userId)
           || (m_doc.effectiveAccess == QStringLiteral("OWNER"))
           || (m_doc.effectiveAccess == QStringLiteral("CO_ADMIN"));
}

bool WriterModule::isReadOnlyMode() const
{
    return (m_doc.effectiveAccess == QStringLiteral("READ"))
           || (m_doc.effectiveAccess == QStringLiteral("ADMIN_VIEW"));
}

void WriterModule::buildUi()
{
    QStyle *st = QApplication::style();

    m_actSave = new QAction(st->standardIcon(QStyle::SP_DialogSaveButton),
                            isOwnerOrCoAdmin() ? tr("Save") : tr("Save Draft"), this);
    m_actSubmitProposal = new QAction(st->standardIcon(QStyle::SP_ArrowForward), tr("Send to Owner"), this);
    m_actExportPdf = new QAction(st->standardIcon(QStyle::SP_FileDialogContentsView), tr("Export PDF"), this);
    m_actPrint = new QAction(st->standardIcon(QStyle::SP_DriveFDIcon), tr("Print"), this);
    m_actUndo = new QAction(st->standardIcon(QStyle::SP_ArrowBack), tr("Undo"), this);
    m_actRedo = new QAction(st->standardIcon(QStyle::SP_ArrowForward), tr("Redo"), this);

    m_actPaste = new QAction(st->standardIcon(QStyle::SP_DialogOpenButton), tr("Paste"), this);
    m_actCut = new QAction(st->standardIcon(QStyle::SP_DialogDiscardButton), tr("Cut"), this);
    m_actCopy = new QAction(st->standardIcon(QStyle::SP_FileIcon), tr("Copy"), this);

    // Compact typographic labels for Ribbon micro-buttons
    m_actGrowFont = new QAction(QStringLiteral("A^"), this);
    m_actShrinkFont = new QAction(QStringLiteral("Av"), this);
    m_actClearFormat = new QAction(QStringLiteral("Aa"), this);
    m_actBold = new QAction(QStringLiteral("B"), this);
    m_actBold->setCheckable(true);
    m_actItalic = new QAction(QStringLiteral("I"), this);
    m_actItalic->setCheckable(true);
    m_actUnderline = new QAction(QStringLiteral("U"), this);
    m_actUnderline->setCheckable(true);
    m_actStrikeOut = new QAction(QStringLiteral("S"), this);
    m_actStrikeOut->setCheckable(true);
    m_actSubscript = new QAction(QStringLiteral("X₂"), this);
    m_actSubscript->setCheckable(true);
    m_actSuperscript = new QAction(QStringLiteral("X²"), this);
    m_actSuperscript->setCheckable(true);
    m_actHighlightColor = new QAction(QStringLiteral("ab"), this);
    m_actTextColor = new QAction(QStringLiteral("A"), this);

    m_actBulletList = new QAction(QStringLiteral("•≡"), this);
    m_actNumberList = new QAction(QStringLiteral("1≡"), this);
    m_actDecreaseIndent = new QAction(QStringLiteral("⇤"), this);
    m_actIncreaseIndent = new QAction(QStringLiteral("⇥"), this);
    m_actLineSpacing = new QAction(QStringLiteral("↕≡"), this);
    m_actShowMarks = new QAction(QStringLiteral("¶"), this);
    m_actShowMarks->setCheckable(true);
    m_actAlignLeft = new QAction(QStringLiteral("≡L"), this);
    m_actAlignCenter = new QAction(QStringLiteral("≡C"), this);
    m_actAlignRight = new QAction(QStringLiteral("≡R"), this);
    m_actAlignJustify = new QAction(QStringLiteral("≡J"), this);
    m_actParagraphShading = new QAction(QStringLiteral("▨"), this);

    // Root workspace container (Dark gray backdrop + Rulers + Centered White Page + Bottom Metrics Bar)
    m_rootWidget = new QWidget();
    m_rootWidget->setStyleSheet(QStringLiteral("QWidget#WriterDarkBackdrop { background-color: #202023; }"));
    QVBoxLayout *rootVBox = new QVBoxLayout(m_rootWidget);
    rootVBox->setContentsMargins(0, 0, 0, 0);
    rootVBox->setSpacing(0);

    m_pageScrollArea = new QScrollArea(m_rootWidget);
    m_pageScrollArea->setObjectName(QStringLiteral("WriterDarkBackdrop"));
    m_pageScrollArea->setWidgetResizable(true);
    m_pageScrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_pageScrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *canvasContainer = new QWidget();
    canvasContainer->setObjectName(QStringLiteral("WriterDarkBackdrop"));
    QGridLayout *canvasGrid = new QGridLayout(canvasContainer);
    canvasGrid->setContentsMargins(24, 14, 24, 24);
    canvasGrid->setSpacing(0);

    m_topRuler = new PageRulerWidget(PageRulerWidget::Horizontal, canvasContainer);
    m_leftRuler = new PageRulerWidget(PageRulerWidget::Vertical, canvasContainer);

    m_editor = new QTextEdit(canvasContainer);
    m_editor->setAcceptRichText(true);
    m_editor->setReadOnly(isReadOnlyMode());
    m_editor->setFixedWidth(794); // Standard A4 width at 96 DPI (210 mm)
    m_editor->setMinimumHeight(1050);
    m_editor->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: #FFFFFF; color: #1E293B; padding: 56px; "
        "border: 1px solid #111827; font-family: 'Liberation Sans', 'Arial', sans-serif; font-size: 11pt; }"));

    canvasGrid->addWidget(m_topRuler, 0, 1);
    canvasGrid->addWidget(m_leftRuler, 1, 0);
    canvasGrid->addWidget(m_editor, 1, 1);

    m_pageScrollArea->setWidget(canvasContainer);
    rootVBox->addWidget(m_pageScrollArea, 1);

    // Bottom status & zoom strip (matching ONLYOFFICE bottom bar)
    QFrame *bottomBar = new QFrame(m_rootWidget);
    bottomBar->setFixedHeight(26);
    bottomBar->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #18181B; border-top: 1px solid #2E2E32; }"
        "QLabel { color: #CBD5E1; font-size: 11px; }"));
    QHBoxLayout *bottomRow = new QHBoxLayout(bottomBar);
    bottomRow->setContentsMargins(12, 0, 12, 0);
    bottomRow->setSpacing(12);

    m_lblPageCount = new QLabel(bottomBar);
    m_lblWordCount = new QLabel(bottomBar);
    m_lblZoomPercent = new QLabel(QStringLiteral("100%"), bottomBar);

    QPushButton *btnZoomOut = new QPushButton(QStringLiteral("-"), bottomBar);
    btnZoomOut->setFixedSize(20, 18);
    QPushButton *btnZoomIn = new QPushButton(QStringLiteral("+"), bottomBar);
    btnZoomIn->setFixedSize(20, 18);

    m_zoomSlider = new QSlider(Qt::Horizontal, bottomBar);
    m_zoomSlider->setRange(50, 200);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(110);

    bottomRow->addWidget(m_lblPageCount);
    bottomRow->addWidget(m_lblWordCount);
    bottomRow->addStretch(1);
    bottomRow->addWidget(btnZoomOut);
    bottomRow->addWidget(m_zoomSlider);
    bottomRow->addWidget(btnZoomIn);
    bottomRow->addWidget(m_lblZoomPercent);

    rootVBox->addWidget(bottomBar);

    const bool editable = !isReadOnlyMode();
    for (QAction *act : { m_actSave, m_actPaste, m_actCut, m_actGrowFont, m_actShrinkFont,
                        m_actClearFormat, m_actBold, m_actItalic, m_actUnderline, m_actStrikeOut,
                        m_actSubscript, m_actSuperscript, m_actHighlightColor, m_actTextColor,
                        m_actBulletList, m_actNumberList, m_actDecreaseIndent, m_actIncreaseIndent,
                        m_actLineSpacing, m_actAlignLeft, m_actAlignCenter, m_actAlignRight,
                        m_actAlignJustify, m_actParagraphShading }) {
        act->setEnabled(editable);
    }

    connect(m_actSave, &QAction::triggered, this, &WriterModule::onSaveTriggered);
    connect(m_actSubmitProposal, &QAction::triggered, this, &WriterModule::onSubmitProposalTriggered);
    connect(m_actExportPdf, &QAction::triggered, this, &WriterModule::onExportPdfTriggered);
    connect(m_actPrint, &QAction::triggered, this, &WriterModule::onPrintTriggered);
    connect(m_actUndo, &QAction::triggered, m_editor, &QTextEdit::undo);
    connect(m_actRedo, &QAction::triggered, m_editor, &QTextEdit::redo);

    connect(m_actPaste, &QAction::triggered, m_editor, &QTextEdit::paste);
    connect(m_actCut, &QAction::triggered, m_editor, &QTextEdit::cut);
    connect(m_actCopy, &QAction::triggered, m_editor, &QTextEdit::copy);

    connect(m_actGrowFont, &QAction::triggered, this, &WriterModule::onGrowFont);
    connect(m_actShrinkFont, &QAction::triggered, this, &WriterModule::onShrinkFont);
    connect(m_actClearFormat, &QAction::triggered, this, &WriterModule::onClearFormatting);
    connect(m_actBold, &QAction::triggered, this, &WriterModule::onToggleBold);
    connect(m_actItalic, &QAction::triggered, this, &WriterModule::onToggleItalic);
    connect(m_actUnderline, &QAction::triggered, this, &WriterModule::onToggleUnderline);
    connect(m_actStrikeOut, &QAction::triggered, this, &WriterModule::onToggleStrikeOut);
    connect(m_actSubscript, &QAction::triggered, this, &WriterModule::onToggleSubscript);
    connect(m_actSuperscript, &QAction::triggered, this, &WriterModule::onToggleSuperscript);
    connect(m_actHighlightColor, &QAction::triggered, this, &WriterModule::onHighlightColorClicked);
    connect(m_actTextColor, &QAction::triggered, this, &WriterModule::onTextColorClicked);

    connect(m_actBulletList, &QAction::triggered, this, &WriterModule::onInsertBulletList);
    connect(m_actNumberList, &QAction::triggered, this, &WriterModule::onInsertNumberedList);
    connect(m_actDecreaseIndent, &QAction::triggered, this, &WriterModule::onDecreaseIndent);
    connect(m_actIncreaseIndent, &QAction::triggered, this, &WriterModule::onIncreaseIndent);
    connect(m_actLineSpacing, &QAction::triggered, this, &WriterModule::onToggleLineSpacing);
    connect(m_actShowMarks, &QAction::toggled, this, &WriterModule::onToggleNonPrintingMarks);
    connect(m_actAlignLeft, &QAction::triggered, this, &WriterModule::onAlignLeft);
    connect(m_actAlignCenter, &QAction::triggered, this, &WriterModule::onAlignCenter);
    connect(m_actAlignRight, &QAction::triggered, this, &WriterModule::onAlignRight);
    connect(m_actAlignJustify, &QAction::triggered, this, &WriterModule::onAlignJustify);
    connect(m_actParagraphShading, &QAction::triggered, this, &WriterModule::onParagraphShadingClicked);

    connect(m_zoomSlider, &QSlider::valueChanged, this, &WriterModule::onZoomSliderChanged);
    connect(btnZoomOut, &QPushButton::clicked, this, [this]() {
        m_zoomSlider->setValue(m_zoomSlider->value() - 10);
    });
    connect(btnZoomIn, &QPushButton::clicked, this, [this]() {
        m_zoomSlider->setValue(m_zoomSlider->value() + 10);
    });

    connect(m_editor, &QTextEdit::cursorPositionChanged, this, &WriterModule::onCursorPositionChanged);
    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &WriterModule::onDocumentContentsChanged);

    // Must be instantiated AFTER m_editor is created
    m_insertHandler = new WriterInsertTabHandler(m_editor, m_doc, m_session, isReadOnlyMode(), this);

    m_drawHandler = new WriterDrawTabHandler(m_editor, isReadOnlyMode(), this);
    connect(m_drawHandler, &WriterDrawTabHandler::drawingCommitted, this, &WriterModule::onDocumentContentsChanged);
}

void WriterModule::buildRibbonWidgets()
{
    auto makeMicroBtn = [](QAction *act, QWidget *parent) {
        QToolButton *b = new QToolButton(parent);
        b->setDefaultAction(act);
        b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        b->setFixedSize(26, 22);
        return b;
    };

    // --- 2-Row Font Group Widget ---
    m_fontGroupWidget = new QWidget();
    QVBoxLayout *fontVBox = new QVBoxLayout(m_fontGroupWidget);
    fontVBox->setContentsMargins(0, 0, 0, 0);
    fontVBox->setSpacing(2);

    QHBoxLayout *fontRow1 = new QHBoxLayout();
    fontRow1->setSpacing(2);
    m_fontCombo = new QFontComboBox(m_fontGroupWidget);
    m_fontCombo->setFixedWidth(120);
    m_fontCombo->setEnabled(!isReadOnlyMode());

    m_fontSizeCombo = new QComboBox(m_fontGroupWidget);
    m_fontSizeCombo->setEditable(true);
    m_fontSizeCombo->setFixedWidth(48);
    m_fontSizeCombo->setEnabled(!isReadOnlyMode());
    for (int sz : { 8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 36 }) {
        m_fontSizeCombo->addItem(QString::number(sz));
    }
    m_fontSizeCombo->setCurrentText(QStringLiteral("11"));

    fontRow1->addWidget(m_fontCombo);
    fontRow1->addWidget(m_fontSizeCombo);
    fontRow1->addWidget(makeMicroBtn(m_actGrowFont, m_fontGroupWidget));
    fontRow1->addWidget(makeMicroBtn(m_actShrinkFont, m_fontGroupWidget));
    fontRow1->addWidget(makeMicroBtn(m_actClearFormat, m_fontGroupWidget));

    QHBoxLayout *fontRow2 = new QHBoxLayout();
    fontRow2->setSpacing(2);
    for (QAction *a : { m_actBold, m_actItalic, m_actUnderline, m_actStrikeOut,
                        m_actSubscript, m_actSuperscript, m_actHighlightColor, m_actTextColor }) {
        fontRow2->addWidget(makeMicroBtn(a, m_fontGroupWidget));
    }
    fontRow2->addStretch(1);

    fontVBox->addLayout(fontRow1);
    fontVBox->addLayout(fontRow2);

    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &WriterModule::onFontFamilyChanged);
    connect(m_fontSizeCombo, &QComboBox::currentTextChanged, this, &WriterModule::onFontSizeChanged);

    // --- 2-Row Paragraph Group Widget ---
    m_paragraphGroupWidget = new QWidget();
    QVBoxLayout *paraVBox = new QVBoxLayout(m_paragraphGroupWidget);
    paraVBox->setContentsMargins(0, 0, 0, 0);
    paraVBox->setSpacing(2);

    QHBoxLayout *paraRow1 = new QHBoxLayout();
    paraRow1->setSpacing(2);
    for (QAction *a : { m_actBulletList, m_actNumberList, m_actDecreaseIndent,
                        m_actIncreaseIndent, m_actLineSpacing, m_actShowMarks }) {
        paraRow1->addWidget(makeMicroBtn(a, m_paragraphGroupWidget));
    }

    QHBoxLayout *paraRow2 = new QHBoxLayout();
    paraRow2->setSpacing(2);
    for (QAction *a : { m_actAlignLeft, m_actAlignCenter, m_actAlignRight,
                        m_actAlignJustify, m_actParagraphShading }) {
        paraRow2->addWidget(makeMicroBtn(a, m_paragraphGroupWidget));
    }
    paraRow2->addStretch(1);

    paraVBox->addLayout(paraRow1);
    paraVBox->addLayout(paraRow2);

    // --- Visual Styles Gallery ---
    m_styleGallery = new StyleGalleryWidget();
    m_styleGallery->setEnabled(!isReadOnlyMode());
    connect(m_styleGallery, &StyleGalleryWidget::stylePresetSelected,
            this, &WriterModule::onStylePresetSelected);
}

void WriterModule::buildInspectorDock()
{
    m_dockWidget = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(m_dockWidget);

    QGroupBox *grpProps = new QGroupBox(tr("Document Governance"), m_dockWidget);
    QVBoxLayout *propLayout = new QVBoxLayout(grpProps);

    m_lblDocMeta = new QLabel(
        tr("<b>Title:</b> %1<br>"
           "<b>Owner:</b> %2<br>"
           "<b>Official Version:</b> v%3")
            .arg(m_doc.title, m_doc.ownerName)
            .arg(m_doc.currentVersion),
        grpProps);
    m_lblDocMeta->setWordWrap(true);
    propLayout->addWidget(m_lblDocMeta);

    m_lblModeBanner = new QLabel(grpProps);
    m_lblModeBanner->setWordWrap(true);
    if (isReadOnlyMode()) {
        m_lblModeBanner->setStyleSheet(QStringLiteral("background-color: #78350F; color: #FEF3C7; padding: 6px; border-radius: 4px;"));
        m_lblModeBanner->setText(tr("Read-Only access."));
    } else if (isOwnerOrCoAdmin()) {
        m_lblModeBanner->setStyleSheet(QStringLiteral("background-color: #14532D; color: #DCFCE7; padding: 6px; border-radius: 4px;"));
        m_lblModeBanner->setText(tr("Direct editing enabled: saving updates the official version."));
    } else {
        m_lblModeBanner->setStyleSheet(QStringLiteral("background-color: #1E3A8A; color: #DBEAFE; padding: 6px; border-radius: 4px;"));
        m_lblModeBanner->setText(tr("Virtual Draft Mode: isolated until approved by owner."));
    }
    propLayout->addWidget(m_lblModeBanner);

    if (!isOwnerOrCoAdmin() && !isReadOnlyMode()) {
        m_btnSubmitProposal = new QPushButton(tr("Submit Changes to Owner for Review"), grpProps);
        propLayout->addWidget(m_btnSubmitProposal);
        connect(m_btnSubmitProposal, &QPushButton::clicked, this, &WriterModule::onSubmitProposalTriggered);
    }

    vbox->addWidget(grpProps);

    QGroupBox *grpAudit = new QGroupBox(tr("Paragraph Forensic History"), m_dockWidget);
    QVBoxLayout *auditLayout = new QVBoxLayout(grpAudit);
    m_listParagraphAudit = new QListWidget(grpAudit);
    auditLayout->addWidget(m_listParagraphAudit);
    vbox->addWidget(grpAudit, 1);
}

void WriterModule::loadBlocksIntoEditor()
{
    m_isLoading = true;
    m_editor->clear();
    m_blockIdsByIndex.clear();

    const QList<DocBlockRecord> blocks = DatabaseManager::instance().loadDocumentBlocks(m_doc.id);
    if (blocks.isEmpty()) {
        m_blockIdsByIndex.append(QUuid::createUuid().toString(QUuid::WithoutBraces));
    } else {
        QTextCursor cursor(m_editor->document());
        for (int i = 0; i < blocks.size(); ++i) {
            if (i > 0) {
                cursor.insertBlock();
            }

            const DocBlockRecord &rec = blocks.at(i);
            QString richHtml;
            if (!rec.formattingJson.isEmpty()) {
                const QJsonDocument jDoc = QJsonDocument::fromJson(rec.formattingJson.toUtf8());
                if (jDoc.isObject()) {
                    richHtml = jDoc.object().value(QStringLiteral("html")).toString();
                }
            }

            if (!richHtml.isEmpty()) {
                cursor.insertHtml(richHtml);
            } else {
                cursor.insertText(rec.contentText);
            }
            m_blockIdsByIndex.append(rec.id);
        }
    }

    m_editor->document()->setModified(false);
    m_isDirty = false;
    m_isLoading = false;
}

void WriterModule::updateWordAndPageMetrics()
{
    const QString text = m_editor->toPlainText().trimmed();
    const int wordCount = text.isEmpty() ? 0 : text.split(QRegExp(QStringLiteral("\\s+")), QtCompat::SkipEmptyParts).size();
    const int pageCount = qMax(1, m_editor->document()->pageCount());

    m_lblPageCount->setText(tr("Page %1 of %2").arg(1).arg(pageCount));
    m_lblWordCount->setText(tr("Word count: %1").arg(wordCount));
}

QList<DocBlockRecord> WriterModule::extractBlocksFromEditor() const
{
    QList<DocBlockRecord> result;
    QTextDocument *doc = m_editor->document();
    int idx = 0;

    for (QTextFrame::iterator it = doc->rootFrame()->begin(); !it.atEnd(); ++it) {
        QTextTable *table = qobject_cast<QTextTable *>(it.currentFrame());
        QTextBlock block = it.currentBlock();

        if (!table && !block.isValid()) {
            continue;
        }

        DocBlockRecord rec;
        rec.id = (idx < m_blockIdsByIndex.size())
                     ? m_blockIdsByIndex.at(idx)
                     : QUuid::createUuid().toString(QUuid::WithoutBraces);
        rec.documentId = m_doc.id;
        rec.orderIndex = (idx + 1) * 1000.0;

        QTextCursor selCursor(doc);
        if (table) {
            rec.blockType = QStringLiteral("TABLE");
            selCursor.setPosition(qMax(0, table->firstPosition() - 1));
            selCursor.setPosition(table->lastPosition() + 1, QTextCursor::KeepAnchor);
            rec.contentText = selCursor.selectedText().replace(QChar::ObjectReplacementCharacter, QLatin1Char(' ')).trimmed();
        } else {
            rec.blockType = QStringLiteral("PARAGRAPH");
            selCursor.setPosition(block.position());
            selCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            rec.contentText = block.text();
        }

        QJsonObject fmtObj;
        fmtObj.insert(QStringLiteral("html"), selCursor.selection().toHtml());
        rec.formattingJson = QString::fromUtf8(QJsonDocument(fmtObj).toJson(QJsonDocument::Compact));

        result.append(rec);
        ++idx;
    }

    return result;
}

void WriterModule::onSaveTriggered()
{
    if (saveChanges()) {
        m_lblDocMeta->setText(
            tr("<b>Title:</b> %1<br>"
               "<b>Owner:</b> %2<br>"
               "<b>Official Version:</b> v%3")
                .arg(m_doc.title, m_doc.ownerName)
                .arg(m_doc.currentVersion));

        QMessageBox::information(
            m_rootWidget,
            tr("Saved"),
            isOwnerOrCoAdmin()
                ? tr("Official document saved (Version v%1).").arg(m_doc.currentVersion)
                : tr("Virtual draft saved. Submit it when ready for owner review."));
    }
}

void WriterModule::onSubmitProposalTriggered()
{
    if (isOwnerOrCoAdmin() || isReadOnlyMode()) {
        return;
    }

    const QList<DocBlockRecord> blocks = extractBlocksFromEditor();
    const QString propTitle = tr("Proposed changes by %1").arg(m_session.fullName);
    QString err;

    if (DatabaseManager::instance().saveOrSubmitWriterProposal(
            m_doc.id, m_session.userId, m_doc.currentVersion, propTitle, blocks, true, err)) {
        m_isDirty = false;
        QMessageBox::information(
            m_rootWidget,
            tr("Proposal Sent"),
            tr("Your changes have been sent to %1 for review and merging.").arg(m_doc.ownerName));
    } else {
        QMessageBox::critical(m_rootWidget, tr("Submission Error"), err);
    }
}

void WriterModule::onExportPdfTriggered()
{
    const QString filePath = QFileDialog::getSaveFileName(
        m_rootWidget, tr("Export Document to PDF"), m_doc.title + QStringLiteral(".pdf"), QStringLiteral("PDF (*.pdf)"));
    if (filePath.isEmpty()) {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    m_editor->document()->print(&printer);
}

void WriterModule::onPrintTriggered()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, m_rootWidget);
    if (dlg.exec() == QDialog::Accepted) {
        m_editor->document()->print(&printer);
    }
}

void WriterModule::onGrowFont()
{
    const qreal current = m_editor->fontPointSize() > 0 ? m_editor->fontPointSize() : 11.0;
    QTextCharFormat fmt;
    fmt.setFontPointSize(current + 2.0);
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onShrinkFont()
{
    const qreal current = m_editor->fontPointSize() > 0 ? m_editor->fontPointSize() : 11.0;
    QTextCharFormat fmt;
    fmt.setFontPointSize(qMax(6.0, current - 2.0));
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onClearFormatting()
{
    QTextCharFormat cleanFmt;
    cleanFmt.setFontFamily(QStringLiteral("Liberation Sans"));
    cleanFmt.setFontPointSize(11.0);
    cleanFmt.setFontWeight(QFont::Normal);
    cleanFmt.setFontItalic(false);
    cleanFmt.setFontUnderline(false);
    cleanFmt.setFontStrikeOut(false);
    cleanFmt.setVerticalAlignment(QTextCharFormat::AlignNormal);
    cleanFmt.setForeground(QColor(0x1E, 0x29, 0x3B));
    cleanFmt.clearBackground();
    m_editor->setCurrentCharFormat(cleanFmt);
}

void WriterModule::onToggleBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_actBold->isChecked() ? QFont::Bold : QFont::Normal);
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onToggleItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(m_actItalic->isChecked());
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onToggleUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(m_actUnderline->isChecked());
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onToggleStrikeOut()
{
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(m_actStrikeOut->isChecked());
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onToggleSubscript()
{
    if (m_actSubscript->isChecked()) {
        m_actSuperscript->setChecked(false);
    }
    QTextCharFormat fmt;
    fmt.setVerticalAlignment(m_actSubscript->isChecked() ? QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal);
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onToggleSuperscript()
{
    if (m_actSuperscript->isChecked()) {
        m_actSubscript->setChecked(false);
    }
    QTextCharFormat fmt;
    fmt.setVerticalAlignment(m_actSuperscript->isChecked() ? QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal);
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onHighlightColorClicked()
{
    const QColor chosen = QColorDialog::getColor(Qt::yellow, m_rootWidget, tr("Select Highlight Color"));
    if (chosen.isValid()) {
        m_editor->setTextBackgroundColor(chosen);
    }
}

void WriterModule::onTextColorClicked()
{
    const QColor chosen = QColorDialog::getColor(m_editor->textColor(), m_rootWidget, tr("Select Text Color"));
    if (chosen.isValid()) {
        m_editor->setTextColor(chosen);
    }
}

void WriterModule::onFontFamilyChanged(const QFont &font)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(font.family());
    m_editor->mergeCurrentCharFormat(fmt);
}

void WriterModule::onFontSizeChanged(const QString &sizeText)
{
    bool ok = false;
    const qreal pts = sizeText.toDouble(&ok);
    if (ok && pts > 0.0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pts);
        m_editor->mergeCurrentCharFormat(fmt);
    }
}

void WriterModule::onAlignLeft() { m_editor->setAlignment(Qt::AlignLeft); }
void WriterModule::onAlignCenter() { m_editor->setAlignment(Qt::AlignCenter); }
void WriterModule::onAlignRight() { m_editor->setAlignment(Qt::AlignRight); }
void WriterModule::onAlignJustify() { m_editor->setAlignment(Qt::AlignJustify); }

void WriterModule::onInsertBulletList()
{
    m_editor->textCursor().insertList(QTextListFormat::ListDisc);
}

void WriterModule::onInsertNumberedList()
{
    m_editor->textCursor().insertList(QTextListFormat::ListDecimal);
}

void WriterModule::onIncreaseIndent()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setIndent(fmt.indent() + 1);
    cursor.setBlockFormat(fmt);
}

void WriterModule::onDecreaseIndent()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setIndent(qMax(0, fmt.indent() - 1));
    cursor.setBlockFormat(fmt);
}

void WriterModule::onToggleLineSpacing()
{
    m_doubleSpacingActive = !m_doubleSpacingActive;
    QTextCursor cursor = m_editor->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setLineHeight(m_doubleSpacingActive ? 150.0 : 100.0, QTextBlockFormat::ProportionalHeight);
    cursor.setBlockFormat(fmt);
}

void WriterModule::onToggleNonPrintingMarks(bool checked)
{
    QTextOption opt = m_editor->document()->defaultTextOption();
    QTextOption::Flags flags = opt.flags();
    if (checked) {
        flags |= QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators;
    } else {
        flags &= ~(QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators);
    }
    opt.setFlags(flags);
    m_editor->document()->setDefaultTextOption(opt);
}

void WriterModule::onParagraphShadingClicked()
{
    const QColor chosen = QColorDialog::getColor(QColor(0xF1, 0xF5, 0xF9), m_rootWidget, tr("Select Paragraph Background"));
    if (chosen.isValid()) {
        QTextCursor cursor = m_editor->textCursor();
        QTextBlockFormat fmt = cursor.blockFormat();
        fmt.setBackground(chosen);
        cursor.setBlockFormat(fmt);
    }
}

void WriterModule::onStylePresetSelected(StyleGalleryWidget::StylePreset preset)
{
    QTextCursor cursor = m_editor->textCursor();
    cursor.select(QTextCursor::BlockUnderCursor);

    QTextCharFormat charFmt;
    QTextBlockFormat blockFmt;

    switch (preset) {
    case StyleGalleryWidget::StylePreset::Normal:
        charFmt.setFontPointSize(11.0);
        charFmt.setFontWeight(QFont::Normal);
        charFmt.setFontItalic(false);
        charFmt.setForeground(QColor(0x1E, 0x29, 0x3B));
        blockFmt.setBottomMargin(8.0);
        break;
    case StyleGalleryWidget::StylePreset::NoSpacing:
        charFmt.setFontPointSize(11.0);
        charFmt.setFontWeight(QFont::Normal);
        charFmt.setForeground(QColor(0x1E, 0x29, 0x3B));
        blockFmt.setBottomMargin(0.0);
        break;
    case StyleGalleryWidget::StylePreset::Heading1:
        charFmt.setFontPointSize(20.0);
        charFmt.setFontWeight(QFont::Bold);
        charFmt.setForeground(QColor(0x1D, 0x4E, 0xD8));
        blockFmt.setTopMargin(12.0);
        blockFmt.setBottomMargin(6.0);
        break;
    case StyleGalleryWidget::StylePreset::Heading2:
        charFmt.setFontPointSize(16.0);
        charFmt.setFontWeight(QFont::Bold);
        charFmt.setForeground(QColor(0x25, 0x63, 0xEB));
        blockFmt.setTopMargin(10.0);
        blockFmt.setBottomMargin(4.0);
        break;
    case StyleGalleryWidget::StylePreset::Heading3:
        charFmt.setFontPointSize(13.0);
        charFmt.setFontWeight(QFont::Bold);
        charFmt.setForeground(QColor(0x3B, 0x82, 0xF6));
        blockFmt.setTopMargin(8.0);
        blockFmt.setBottomMargin(4.0);
        break;
    case StyleGalleryWidget::StylePreset::Heading4:
        charFmt.setFontPointSize(11.5);
        charFmt.setFontWeight(QFont::Bold);
        charFmt.setFontItalic(true);
        charFmt.setForeground(QColor(0x25, 0x63, 0xEB));
        blockFmt.setTopMargin(6.0);
        blockFmt.setBottomMargin(2.0);
        break;
    }

    cursor.setBlockFormat(blockFmt);
    cursor.mergeCharFormat(charFmt);
    m_editor->mergeCurrentCharFormat(charFmt);
}

void WriterModule::onZoomSliderChanged(int percent)
{
    const int delta = percent - m_currentZoomPercent;
    m_currentZoomPercent = percent;

    m_lblZoomPercent->setText(QStringLiteral("%1%").arg(percent));
    const double factor = percent / 100.0;
    m_editor->setFixedWidth(static_cast<int>(794 * factor));
    m_topRuler->setZoomFactor(factor);
    m_leftRuler->setZoomFactor(factor);

    if (delta > 0) {
        m_editor->zoomIn(delta / 10);
    } else if (delta < 0) {
        m_editor->zoomOut((-delta) / 10);
    }
}

void WriterModule::onCursorPositionChanged()
{
    const QTextCharFormat fmt = m_editor->currentCharFormat();
    m_actBold->setChecked(fmt.fontWeight() >= QFont::Bold);
    m_actItalic->setChecked(fmt.fontItalic());
    m_actUnderline->setChecked(fmt.fontUnderline());
    m_actStrikeOut->setChecked(fmt.fontStrikeOut());
    m_actSubscript->setChecked(fmt.verticalAlignment() == QTextCharFormat::AlignSubScript);
    m_actSuperscript->setChecked(fmt.verticalAlignment() == QTextCharFormat::AlignSuperScript);

    const int blockNum = m_editor->textCursor().blockNumber();
    m_listParagraphAudit->clear();

    if (blockNum >= 0 && blockNum < m_blockIdsByIndex.size()) {
        const QString blockId = m_blockIdsByIndex.at(blockNum);
        const QList<BlockAuditRecord> history = DatabaseManager::instance().loadBlockAuditHistory(blockId);
        if (history.isEmpty()) {
            m_listParagraphAudit->addItem(tr("No prior revisions recorded for this paragraph."));
        } else {
            for (const BlockAuditRecord &rec : history) {
                m_listParagraphAudit->addItem(
                    tr("v%1 - %2 (%3)\n\"%4\"").arg(rec.versionNum).arg(rec.changedByName, rec.changedAt, rec.newValue.left(60)));
            }
        }
    }
}

void WriterModule::onDocumentContentsChanged()
{
    updateWordAndPageMetrics();
    if (m_isLoading || isReadOnlyMode()) {
        return;
    }
    m_isDirty = true;
}
