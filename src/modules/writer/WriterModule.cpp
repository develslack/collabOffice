#include "WriterModule.h"

#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextListFormat>
#include <QFontComboBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
    , m_rootWidget(nullptr)
    , m_editor(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeCombo(nullptr)
    , m_dockWidget(nullptr)
    , m_lblDocMeta(nullptr)
    , m_lblModeBanner(nullptr)
    , m_btnSubmitProposal(nullptr)
    , m_listParagraphAudit(nullptr)
{
    buildUi();
    buildInspectorDock();
    loadBlocksIntoEditor();
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
    CommandGroupSpec fileGrp;
    fileGrp.tabTitle = tr("Home");
    fileGrp.groupTitle = tr("Document & Sync");
    fileGrp.actions = { m_actSave };
    if (!isOwnerOrCoAdmin() && !isReadOnlyMode()) {
        fileGrp.actions.append(m_actSubmitProposal);
    }
    fileGrp.actions << m_actExportPdf << m_actPrint;

    CommandGroupSpec editGrp;
    editGrp.tabTitle = tr("Home");
    editGrp.groupTitle = tr("History");
    editGrp.actions = { m_actUndo, m_actRedo };

    CommandGroupSpec fontGrp;
    fontGrp.tabTitle = tr("Home");
    fontGrp.groupTitle = tr("Font");
    fontGrp.actions = { m_actBold, m_actItalic, m_actUnderline, m_actTextColor };

    CommandGroupSpec paraGrp;
    paraGrp.tabTitle = tr("Home");
    paraGrp.groupTitle = tr("Paragraph");
    paraGrp.actions = {
        m_actAlignLeft, m_actAlignCenter, m_actAlignRight, m_actAlignJustify,
        m_actBulletList, m_actNumberList
    };

    return { fileGrp, editGrp, fontGrp, paraGrp };
}

QString WriterModule::statusSummary() const
{
    if (isReadOnlyMode()) {
        return tr("Read-Only Mode | Official Version v%1 | Owner: %2").arg(m_doc.currentVersion).arg(m_doc.ownerName);
    }
    if (isOwnerOrCoAdmin()) {
        return tr("Direct Official Editing | Version v%1 | Owner: %2").arg(m_doc.currentVersion).arg(m_doc.ownerName);
    }
    return tr("Virtual Contributor Draft (Base v%1) | Changes require Owner (%2) approval").arg(m_doc.currentVersion).arg(m_doc.ownerName);
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
    m_actSave->setText(isOwnerOrCoAdmin() ? tr("Save Official") : tr("Save Draft"));
    m_actSubmitProposal->setText(tr("Send to Owner"));
    m_actExportPdf->setText(tr("Export PDF"));
    m_actPrint->setText(tr("Print"));
    m_actUndo->setText(tr("Undo"));
    m_actRedo->setText(tr("Redo"));
    m_actBold->setText(tr("Bold"));
    m_actItalic->setText(tr("Italic"));
    m_actUnderline->setText(tr("Underline"));
    m_actTextColor->setText(tr("Text Color"));
    m_actAlignLeft->setText(tr("Align Left"));
    m_actAlignCenter->setText(tr("Center"));
    m_actAlignRight->setText(tr("Align Right"));
    m_actAlignJustify->setText(tr("Justify"));
    m_actBulletList->setText(tr("Bullets"));
    m_actNumberList->setText(tr("Numbering"));

    if (m_btnSubmitProposal) {
        m_btnSubmitProposal->setText(tr("Submit Changes to Owner for Review"));
    }
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
                            isOwnerOrCoAdmin() ? tr("Save Official") : tr("Save Draft"), this);
    m_actSubmitProposal = new QAction(st->standardIcon(QStyle::SP_ArrowForward), tr("Send to Owner"), this);
    m_actExportPdf = new QAction(st->standardIcon(QStyle::SP_FileDialogContentsView), tr("Export PDF"), this);
    m_actPrint = new QAction(st->standardIcon(QStyle::SP_DriveFDIcon), tr("Print"), this);
    m_actUndo = new QAction(st->standardIcon(QStyle::SP_ArrowBack), tr("Undo"), this);
    m_actRedo = new QAction(st->standardIcon(QStyle::SP_ArrowForward), tr("Redo"), this);

    m_actBold = new QAction(tr("Bold"), this);
    m_actBold->setCheckable(true);
    m_actItalic = new QAction(tr("Italic"), this);
    m_actItalic->setCheckable(true);
    m_actUnderline = new QAction(tr("Underline"), this);
    m_actUnderline->setCheckable(true);
    m_actTextColor = new QAction(tr("Text Color"), this);

    m_actAlignLeft = new QAction(tr("Align Left"), this);
    m_actAlignCenter = new QAction(tr("Center"), this);
    m_actAlignRight = new QAction(tr("Align Right"), this);
    m_actAlignJustify = new QAction(tr("Justify"), this);
    m_actBulletList = new QAction(tr("Bullets"), this);
    m_actNumberList = new QAction(tr("Numbering"), this);

    m_rootWidget = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(m_rootWidget);
    vbox->setContentsMargins(12, 8, 12, 12);

    // Compact font selector strip above the page canvas
    QHBoxLayout *fontStrip = new QHBoxLayout();
    m_fontCombo = new QFontComboBox(m_rootWidget);
    m_fontSizeCombo = new QComboBox(m_rootWidget);
    m_fontSizeCombo->setEditable(true);
    for (int sz : { 8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 36 }) {
        m_fontSizeCombo->addItem(QString::number(sz));
    }
    m_fontSizeCombo->setCurrentText(QStringLiteral("11"));

    fontStrip->addWidget(new QLabel(tr("Font:"), m_rootWidget));
    fontStrip->addWidget(m_fontCombo);
    fontStrip->addWidget(new QLabel(tr("Size:"), m_rootWidget));
    fontStrip->addWidget(m_fontSizeCombo);
    fontStrip->addStretch(1);
    vbox->addLayout(fontStrip);

    // Document canvas styled like a corporate page
    m_editor = new QTextEdit(m_rootWidget);
    m_editor->setAcceptRichText(true);
    m_editor->setReadOnly(isReadOnlyMode());
    m_editor->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: #FFFFFF; color: #1E293B; padding: 28px; "
        "border: 1px solid #CBD5E1; font-family: 'Liberation Sans', 'Arial', sans-serif; font-size: 11pt; }"));
    vbox->addWidget(m_editor, 1);

    // Disable editing actions in read-only mode
    const bool editable = !isReadOnlyMode();
    m_actSave->setEnabled(editable);
    m_actBold->setEnabled(editable);
    m_actItalic->setEnabled(editable);
    m_actUnderline->setEnabled(editable);
    m_actTextColor->setEnabled(editable);
    m_actAlignLeft->setEnabled(editable);
    m_actAlignCenter->setEnabled(editable);
    m_actAlignRight->setEnabled(editable);
    m_actAlignJustify->setEnabled(editable);
    m_actBulletList->setEnabled(editable);
    m_actNumberList->setEnabled(editable);
    m_fontCombo->setEnabled(editable);
    m_fontSizeCombo->setEnabled(editable);

    connect(m_actSave, &QAction::triggered, this, &WriterModule::onSaveTriggered);
    connect(m_actSubmitProposal, &QAction::triggered, this, &WriterModule::onSubmitProposalTriggered);
    connect(m_actExportPdf, &QAction::triggered, this, &WriterModule::onExportPdfTriggered);
    connect(m_actPrint, &QAction::triggered, this, &WriterModule::onPrintTriggered);
    connect(m_actUndo, &QAction::triggered, m_editor, &QTextEdit::undo);
    connect(m_actRedo, &QAction::triggered, m_editor, &QTextEdit::redo);

    connect(m_actBold, &QAction::triggered, this, &WriterModule::onToggleBold);
    connect(m_actItalic, &QAction::triggered, this, &WriterModule::onToggleItalic);
    connect(m_actUnderline, &QAction::triggered, this, &WriterModule::onToggleUnderline);
    connect(m_actTextColor, &QAction::triggered, this, &WriterModule::onTextColorClicked);
    connect(m_actAlignLeft, &QAction::triggered, this, &WriterModule::onAlignLeft);
    connect(m_actAlignCenter, &QAction::triggered, this, &WriterModule::onAlignCenter);
    connect(m_actAlignRight, &QAction::triggered, this, &WriterModule::onAlignRight);
    connect(m_actAlignJustify, &QAction::triggered, this, &WriterModule::onAlignJustify);
    connect(m_actBulletList, &QAction::triggered, this, &WriterModule::onInsertBulletList);
    connect(m_actNumberList, &QAction::triggered, this, &WriterModule::onInsertNumberedList);

    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &WriterModule::onFontFamilyChanged);
    connect(m_fontSizeCombo, &QComboBox::currentTextChanged, this, &WriterModule::onFontSizeChanged);

    connect(m_editor, &QTextEdit::cursorPositionChanged, this, &WriterModule::onCursorPositionChanged);
    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &WriterModule::onDocumentContentsChanged);
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
        m_lblModeBanner->setStyleSheet(QStringLiteral("background-color: #FEF3C7; color: #92400E; padding: 6px; border-radius: 4px;"));
        m_lblModeBanner->setText(tr("You have Read-Only access to this document."));
    } else if (isOwnerOrCoAdmin()) {
        m_lblModeBanner->setStyleSheet(QStringLiteral("background-color: #DCFCE7; color: #166534; padding: 6px; border-radius: 4px;"));
        m_lblModeBanner->setText(tr("Direct editing enabled: saving updates the official version."));
    } else {
        m_lblModeBanner->setStyleSheet(QStringLiteral("background-color: #DBEAFE; color: #1E40AF; padding: 6px; border-radius: 4px;"));
        m_lblModeBanner->setText(tr("Virtual Draft Mode: your edits are isolated until approved by the document owner."));
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
            cursor.insertText(blocks.at(i).contentText);
            m_blockIdsByIndex.append(blocks.at(i).id);
        }
    }

    m_editor->document()->setModified(false);
    m_isDirty = false;
    m_isLoading = false;
}

QList<DocBlockRecord> WriterModule::extractBlocksFromEditor() const
{
    QList<DocBlockRecord> result;
    QTextDocument *doc = m_editor->document();
    int idx = 0;

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next(), ++idx) {
        DocBlockRecord rec;
        rec.id = (idx < m_blockIdsByIndex.size())
                     ? m_blockIdsByIndex.at(idx)
                     : QUuid::createUuid().toString(QUuid::WithoutBraces);
        rec.documentId = m_doc.id;
        rec.orderIndex = (idx + 1) * 1000.0;
        rec.blockType = QStringLiteral("PARAGRAPH");
        rec.contentText = block.text();
        result.append(rec);
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
                : tr("Your virtual draft has been saved locally/remotely. Submit it when ready for owner review."));
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
            tr("Your changes have been sent to %1 for evaluation and merging.").arg(m_doc.ownerName));
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

void WriterModule::onCursorPositionChanged()
{
    const QTextCharFormat fmt = m_editor->currentCharFormat();
    m_actBold->setChecked(fmt.fontWeight() >= QFont::Bold);
    m_actItalic->setChecked(fmt.fontItalic());
    m_actUnderline->setChecked(fmt.fontUnderline());

    // Load forensic audit history for the paragraph currently under the cursor
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
    if (m_isLoading || isReadOnlyMode()) {
        return;
    }
    m_isDirty = true;
}
