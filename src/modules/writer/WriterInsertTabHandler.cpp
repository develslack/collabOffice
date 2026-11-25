#include "WriterInsertTabHandler.h"
#include "BootstrapManager.h"
#include "QtCompat.h"

#include <QTextEdit>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextTableFormat>
#include <QTextTableCellFormat>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QAction>
#include <QDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QApplication>
#include <QStyle>

WriterInsertTabHandler::WriterInsertTabHandler(QTextEdit *editor,
                                               const DocumentSummary &docSummary,
                                               const UserSession &session,
                                               bool readOnlyMode,
                                               QObject *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_doc(docSummary)
    , m_session(session)
    , m_readOnly(readOnlyMode)
{
    createActions();
    retranslateUi();
}

void WriterInsertTabHandler::setEditor(QTextEdit *editor)
{
    m_editor = editor;
}

void WriterInsertTabHandler::createActions()
{
    QStyle *st = QApplication::style();

    m_actBlankPage      = new QAction(st->standardIcon(QStyle::SP_FileIcon), QString(), this);
    m_actPageBreak      = new QAction(st->standardIcon(QStyle::SP_ArrowDown), QString(), this);
    m_actTable          = new QAction(st->standardIcon(QStyle::SP_FileDialogDetailedView), QString(), this);
    m_actImage          = new QAction(st->standardIcon(QStyle::SP_DriveHDIcon), QString(), this);
    m_actShape          = new QAction(st->standardIcon(QStyle::SP_DialogApplyButton), QString(), this);
    m_actDiagram        = new QAction(st->standardIcon(QStyle::SP_ComputerIcon), QString(), this);
    m_actComment        = new QAction(st->standardIcon(QStyle::SP_MessageBoxInformation), QString(), this);
    m_actLink           = new QAction(st->standardIcon(QStyle::SP_DriveNetIcon), QString(), this);
    m_actHeaderFooter   = new QAction(st->standardIcon(QStyle::SP_TitleBarUnshadeButton), QString(), this);
    m_actPageNumber     = new QAction(st->standardIcon(QStyle::SP_FileDialogInfoView), QString(), this);
    m_actTextBox        = new QAction(st->standardIcon(QStyle::SP_DialogSaveButton), QString(), this);
    m_actTextArt        = new QAction(st->standardIcon(QStyle::SP_DialogYesButton), QString(), this);
    m_actDateTime       = new QAction(st->standardIcon(QStyle::SP_BrowserReload), QString(), this);
    m_actTextFromFile   = new QAction(st->standardIcon(QStyle::SP_DirOpenIcon), QString(), this);
    m_actField          = new QAction(st->standardIcon(QStyle::SP_CommandLink), QString(), this);
    m_actEquation       = new QAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QString(), this);
    m_actSymbol         = new QAction(st->standardIcon(QStyle::SP_DialogHelpButton), QString(), this);
    m_actContentControl = new QAction(st->standardIcon(QStyle::SP_DialogOkButton), QString(), this);

    const bool editable = !m_readOnly;
    for (QAction *act : { m_actBlankPage, m_actPageBreak, m_actTable, m_actImage, m_actShape,
                        m_actDiagram, m_actComment, m_actLink, m_actHeaderFooter, m_actPageNumber,
                        m_actTextBox, m_actTextArt, m_actDateTime, m_actTextFromFile, m_actField,
                        m_actEquation, m_actSymbol, m_actContentControl }) {
        act->setEnabled(editable);
    }

    connect(m_actBlankPage, &QAction::triggered, this, &WriterInsertTabHandler::onInsertBlankPage);
    connect(m_actPageBreak, &QAction::triggered, this, &WriterInsertTabHandler::onInsertPageBreak);
    connect(m_actTable, &QAction::triggered, this, &WriterInsertTabHandler::onInsertTable);
    connect(m_actImage, &QAction::triggered, this, &WriterInsertTabHandler::onInsertImage);
    connect(m_actShape, &QAction::triggered, this, &WriterInsertTabHandler::onInsertShape);
    connect(m_actDiagram, &QAction::triggered, this, &WriterInsertTabHandler::onInsertDiagram);
    connect(m_actComment, &QAction::triggered, this, &WriterInsertTabHandler::onInsertComment);
    connect(m_actLink, &QAction::triggered, this, &WriterInsertTabHandler::onInsertLink);
    connect(m_actHeaderFooter, &QAction::triggered, this, &WriterInsertTabHandler::onInsertHeaderFooter);
    connect(m_actPageNumber, &QAction::triggered, this, &WriterInsertTabHandler::onInsertPageNumber);
    connect(m_actTextBox, &QAction::triggered, this, &WriterInsertTabHandler::onInsertTextBox);
    connect(m_actTextArt, &QAction::triggered, this, &WriterInsertTabHandler::onInsertTextArt);
    connect(m_actDateTime, &QAction::triggered, this, &WriterInsertTabHandler::onInsertDateTime);
    connect(m_actTextFromFile, &QAction::triggered, this, &WriterInsertTabHandler::onInsertTextFromFile);
    connect(m_actField, &QAction::triggered, this, &WriterInsertTabHandler::onInsertField);
    connect(m_actEquation, &QAction::triggered, this, &WriterInsertTabHandler::onInsertEquation);
    connect(m_actSymbol, &QAction::triggered, this, &WriterInsertTabHandler::onInsertSymbol);
    connect(m_actContentControl, &QAction::triggered, this, &WriterInsertTabHandler::onInsertContentControl);
}

QList<CommandGroupSpec> WriterInsertTabHandler::commandGroups() const
{
    const QString tabName = tr("Insert");

    CommandGroupSpec pagesGrp;
    pagesGrp.tabTitle = tabName;
    pagesGrp.groupTitle = tr("Pages");
    pagesGrp.actions = { m_actBlankPage, m_actPageBreak };

    CommandGroupSpec tablesGrp;
    tablesGrp.tabTitle = tabName;
    tablesGrp.groupTitle = tr("Tables");
    tablesGrp.actions = { m_actTable };

    CommandGroupSpec illusGrp;
    illusGrp.tabTitle = tabName;
    illusGrp.groupTitle = tr("Illustrations");
    illusGrp.actions = { m_actImage, m_actShape, m_actDiagram };

    CommandGroupSpec linksGrp;
    linksGrp.tabTitle = tabName;
    linksGrp.groupTitle = tr("Links");
    linksGrp.actions = { m_actComment, m_actLink };

    CommandGroupSpec headerGrp;
    headerGrp.tabTitle = tabName;
    headerGrp.groupTitle = tr("Header & Footer");
    headerGrp.actions = { m_actHeaderFooter, m_actPageNumber };

    CommandGroupSpec textGrp;
    textGrp.tabTitle = tabName;
    textGrp.groupTitle = tr("Text");
    textGrp.actions = { m_actTextBox, m_actTextArt, m_actDateTime, m_actTextFromFile, m_actField };

    CommandGroupSpec symbolsGrp;
    symbolsGrp.tabTitle = tabName;
    symbolsGrp.groupTitle = tr("Symbols");
    symbolsGrp.actions = { m_actEquation, m_actSymbol, m_actContentControl };

    return { pagesGrp, tablesGrp, illusGrp, linksGrp, headerGrp, textGrp, symbolsGrp };
}

void WriterInsertTabHandler::retranslateUi()
{
    m_actBlankPage->setText(tr("Blank Page"));
    m_actPageBreak->setText(tr("Breaks"));
    m_actTable->setText(tr("Table"));
    m_actImage->setText(tr("Image"));
    m_actShape->setText(tr("Shape"));
    m_actDiagram->setText(tr("Chart"));
    m_actComment->setText(tr("Comment"));
    m_actLink->setText(tr("Link"));
    m_actHeaderFooter->setText(tr("Header/Footer"));
    m_actPageNumber->setText(tr("Page Number"));
    m_actTextBox->setText(tr("Text Box"));
    m_actTextArt->setText(tr("Text Art"));
    m_actDateTime->setText(tr("Date & Time"));
    m_actTextFromFile->setText(tr("Text from File"));
    m_actField->setText(tr("Field"));
    m_actEquation->setText(tr("Equation"));
    m_actSymbol->setText(tr("Symbol"));
    m_actContentControl->setText(tr("Controls"));
}

void WriterInsertTabHandler::onInsertBlankPage()
{
    if (!m_editor) {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    QTextBlockFormat pageBreakFmt;
    pageBreakFmt.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
    cursor.insertBlock(pageBreakFmt);
    cursor.insertBlock();
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
}

void WriterInsertTabHandler::onInsertPageBreak()
{
    if (!m_editor) {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();
    QTextBlockFormat pageBreakFmt;
    pageBreakFmt.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
    cursor.insertBlock(pageBreakFmt);
    m_editor->setTextCursor(cursor);
}

void WriterInsertTabHandler::onInsertTable()
{
    if (!m_editor) {
        return;
    }

    QDialog dlg(m_editor);
    dlg.setWindowTitle(tr("Insert Table"));
    dlg.setMinimumWidth(340);

    QFormLayout *form = new QFormLayout(&dlg);
    QSpinBox *spinRows = new QSpinBox(&dlg);
    spinRows->setRange(1, 100);
    spinRows->setValue(3);

    QSpinBox *spinCols = new QSpinBox(&dlg);
    spinCols->setRange(1, 20);
    spinCols->setValue(4);

    QCheckBox *chkHeaderRow = new QCheckBox(tr("Style first row as corporate header"), &dlg);
    chkHeaderRow->setChecked(true);

    QPushButton *btnOk = new QPushButton(tr("Insert Table"), &dlg);

    form->addRow(tr("Number of Rows:"), spinRows);
    form->addRow(tr("Number of Columns:"), spinCols);
    form->addRow(chkHeaderRow);
    form->addRow(btnOk);

    connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const int rows = spinRows->value();
    const int cols = spinCols->value();

    QTextTableFormat tableFmt;
    tableFmt.setBorder(1.0);
    tableFmt.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    tableFmt.setBorderBrush(QColor(0x94, 0xA3, 0xB8));
    tableFmt.setCellPadding(6.0);
    tableFmt.setCellSpacing(0.0);
    tableFmt.setWidth(QTextLength(QTextLength::PercentageLength, 100.0));

    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    QTextTable *table = cursor.insertTable(rows, cols, tableFmt);

    if (table && chkHeaderRow->isChecked()) {
        QTextCharFormat headerCharFmt;
        headerCharFmt.setFontWeight(QFont::Bold);
        headerCharFmt.setForeground(QColor(0x1E, 0x29, 0x3B));

        QTextTableCellFormat headerCellFmt;
        headerCellFmt.setBackground(QColor(0xE2, 0xE8, 0xF0));

        for (int c = 0; c < cols; ++c) {
            QTextTableCell cell = table->cellAt(0, c);
            if (!cell.isValid()) {
                continue;
            }
            cell.setFormat(headerCellFmt);
            QTextCursor cellCur = cell.firstCursorPosition();
            cellCur.mergeCharFormat(headerCharFmt);
        }
    }
    cursor.endEditBlock();
}

void WriterInsertTabHandler::onInsertImage()
{
    if (!m_editor) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        m_editor,
        tr("Select Image to Insert"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"));

    if (filePath.isEmpty()) {
        return;
    }

    QImage img(filePath);
    if (img.isNull()) {
        QMessageBox::warning(m_editor, tr("Invalid Image"), tr("Could not load the selected image file."));
        return;
    }

    // Auto-fit image within A4 printable width (640 px max) so it never overflows page margins
    if (img.width() > 640) {
        img = img.scaledToWidth(640, Qt::SmoothTransformation);
    }

    // Embed as Base64 PNG data URI so all network workstations display the image without broken paths
    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");

    const QString html = QStringLiteral("<p><img src=\"data:image/png;base64,%1\" width=\"%2\" height=\"%3\" /></p>")
                             .arg(QString::fromLatin1(pngBytes.toBase64()))
                             .arg(img.width())
                             .arg(img.height());

    m_editor->textCursor().insertHtml(html);
}

void WriterInsertTabHandler::onInsertShape()
{
    if (!m_editor) {
        return;
    }

    QStringList shapes = {
        tr("Corporate Banner Box"),
        tr("Information Callout Frame"),
        tr("Warning Highlight Box"),
        tr("Horizontal Section Divider")
    };

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        m_editor, tr("Insert Shape / Callout"), tr("Select shape style:"), shapes, 0, false, &ok);
    if (!ok || chosen.isEmpty()) {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();
    if (chosen == shapes.at(3)) {
        cursor.insertHtml(QStringLiteral("<hr style=\"border: 1px solid #3B82F6;\" /><p></p>"));
        return;
    }

    QString bgHex = QStringLiteral("#EFF6FF");
    QString borderHex = QStringLiteral("#2563EB");
    if (chosen == shapes.at(0)) {
        bgHex = QStringLiteral("#1E293B");
        borderHex = QStringLiteral("#0F172A");
    } else if (chosen == shapes.at(2)) {
        bgHex = QStringLiteral("#FEF3C7");
        borderHex = QStringLiteral("#D97706");
    }

    const QString textColor = (chosen == shapes.at(0)) ? QStringLiteral("#FFFFFF") : QStringLiteral("#1E293B");
    const QString html = QStringLiteral(
        "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"10\" style=\"border: 2px solid %1; background-color: %2;\">"
        "<tr><td style=\"color: %3;\">%4</td></tr></table><p></p>")
                             .arg(borderHex, bgHex, textColor, tr("Enter text inside shape..."));
    cursor.insertHtml(html);
}

void WriterInsertTabHandler::onInsertDiagram()
{
    if (!m_editor) {
        return;
    }

    QDialog dlg(m_editor);
    dlg.setWindowTitle(tr("Insert Process Diagram"));
    dlg.setMinimumWidth(400);

    QFormLayout *form = new QFormLayout(&dlg);
    QLineEdit *editStep1 = new QLineEdit(tr("1. Draft"), &dlg);
    QLineEdit *editStep2 = new QLineEdit(tr("2. Review"), &dlg);
    QLineEdit *editStep3 = new QLineEdit(tr("3. Approved"), &dlg);
    QPushButton *btnInsert = new QPushButton(tr("Insert Diagram"), &dlg);

    form->addRow(tr("Stage 1:"), editStep1);
    form->addRow(tr("Stage 2:"), editStep2);
    form->addRow(tr("Stage 3:"), editStep3);
    form->addRow(btnInsert);

    connect(btnInsert, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const QString html = QStringLiteral(
        "<table width=\"100%\" cellspacing=\"6\" cellpadding=\"10\"><tr>"
        "<td align=\"center\" style=\"background-color:#DBEAFE; border:1px solid #2563EB; color:#1E3A8A; font-weight:bold;\">%1</td>"
        "<td align=\"center\" style=\"font-weight:bold; color:#64748B;\">&#10140;</td>"
        "<td align=\"center\" style=\"background-color:#FEF3C7; border:1px solid #D97706; color:#92400E; font-weight:bold;\">%2</td>"
        "<td align=\"center\" style=\"font-weight:bold; color:#64748B;\">&#10140;</td>"
        "<td align=\"center\" style=\"background-color:#DCFCE7; border:1px solid #16A34A; color:#166534; font-weight:bold;\">%3</td>"
        "</tr></table><p></p>")
                             .arg(editStep1->text().toHtmlEscaped(),
                                  editStep2->text().toHtmlEscaped(),
                                  editStep3->text().toHtmlEscaped());
    m_editor->textCursor().insertHtml(html);
}

void WriterInsertTabHandler::onInsertComment()
{
    if (!m_editor) {
        return;
    }

    bool ok = false;
    const QString note = QInputDialog::getMultiLineText(
        m_editor,
        tr("Insert Review Comment"),
        tr("Annotation by %1:").arg(m_session.fullName),
        QString(),
        &ok).trimmed();

    if (!ok || note.isEmpty()) {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    if (cursor.hasSelection()) {
        QTextCharFormat highlightFmt;
        highlightFmt.setBackground(QColor(0xFE, 0xF0, 0x8A));
        cursor.mergeCharFormat(highlightFmt);
        cursor.clearSelection();
    }

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    const QString badgeHtml = QStringLiteral(
        " <span style=\"background-color:#FEF9C3; color:#854D0E; font-size:9pt;\">[%1 (%2): %3]</span> ")
                                  .arg(m_session.fullName.toHtmlEscaped(), stamp, note.toHtmlEscaped());
    cursor.insertHtml(badgeHtml);
    cursor.endEditBlock();
}

void WriterInsertTabHandler::onInsertLink()
{
    if (!m_editor) {
        return;
    }

    QDialog dlg(m_editor);
    dlg.setWindowTitle(tr("Insert Hyperlink or CollabOffice Document Reference"));
    dlg.setMinimumWidth(440);

    QFormLayout *form = new QFormLayout(&dlg);
    QComboBox *comboType = new QComboBox(&dlg);
    comboType->addItem(tr("External Web URL (https://...)"), QStringLiteral("URL"));
    comboType->addItem(tr("CollabOffice Internal Document (Unbreakable ID Link)"), QStringLiteral("INTERNAL_DOC"));

    QLineEdit *editLabel = new QLineEdit(&dlg);
    QLineEdit *editUrl = new QLineEdit(QStringLiteral("https://"), &dlg);
    QComboBox *comboDocs = new QComboBox(&dlg);

    for (const DocumentSummary &d : DatabaseManager::instance().listAccessibleDocuments(m_session)) {
        if (d.id != m_doc.id) {
            comboDocs->addItem(QStringLiteral("%1 (v%2 - %3)").arg(d.title).arg(d.currentVersion).arg(d.ownerName), d.id);
        }
    }
    comboDocs->setVisible(false);

    connect(comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&](int idx) {
        const bool isInternal = (comboType->itemData(idx).toString() == QStringLiteral("INTERNAL_DOC"));
        editUrl->setVisible(!isInternal);
        comboDocs->setVisible(isInternal);
        if (isInternal && editLabel->text().isEmpty() && comboDocs->count() > 0) {
            editLabel->setText(comboDocs->currentText());
        }
    });

    QPushButton *btnInsert = new QPushButton(tr("Insert Link"), &dlg);
    form->addRow(tr("Link Type:"), comboType);
    form->addRow(tr("Display Text:"), editLabel);
    form->addRow(tr("Target URL:"), editUrl);
    form->addRow(tr("Target Document:"), comboDocs);
    form->addRow(btnInsert);

    connect(btnInsert, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const bool isInternal = (comboType->currentData().toString() == QStringLiteral("INTERNAL_DOC"));
    if (isInternal && comboDocs->count() == 0) {
        QMessageBox::information(m_editor, tr("No Other Documents"),
                                 tr("There are no other accessible documents available to link."));
        return;
    }

    const QString targetHref = isInternal
                                   ? QStringLiteral("collaboffice://doc/%1").arg(comboDocs->currentData().toString())
                                   : editUrl->text().trimmed();
    QString label = editLabel->text().trimmed();
    if (label.isEmpty()) {
        label = isInternal ? comboDocs->currentText() : targetHref;
    }

    const QString html = QStringLiteral("<a href=\"%1\" style=\"color:#2563EB; text-decoration:underline;\">%2</a>")
                             .arg(targetHref.toHtmlEscaped(), label.toHtmlEscaped());
    m_editor->textCursor().insertHtml(html);
}

void WriterInsertTabHandler::onInsertHeaderFooter()
{
    if (!m_editor) {
        return;
    }

    bool ok = false;
    const QString headerText = QInputDialog::getText(
        m_editor,
        tr("Document Header / Banner"),
        tr("Enter header text to insert at the top of the document:"),
        QLineEdit::Normal,
        QStringLiteral("%1 — CollabOffice").arg(m_doc.title),
        &ok).trimmed();

    if (!ok || headerText.isEmpty()) {
        return;
    }

    QTextCursor topCursor(m_editor->document());
    topCursor.movePosition(QTextCursor::Start);
    const QString html = QStringLiteral(
        "<div style=\"color:#64748B; font-size:9pt; border-bottom:1px solid #CBD5E1; padding-bottom:4px;\">%1</div><p></p>")
                             .arg(headerText.toHtmlEscaped());
    topCursor.insertHtml(html);
}

void WriterInsertTabHandler::onInsertPageNumber()
{
    if (!m_editor) {
        return;
    }

    const int pageCount = qMax(1, m_editor->document()->pageCount());
    const QString badge = QStringLiteral("<span style=\"color:#475569; font-size:9pt;\">[%1]</span>")
                              .arg(tr("Page 1 of %1").arg(pageCount));
    m_editor->textCursor().insertHtml(badge);
}

void WriterInsertTabHandler::onInsertTextBox()
{
    if (!m_editor) {
        return;
    }

    const QString html = QStringLiteral(
        "<table width=\"85%\" align=\"center\" cellspacing=\"0\" cellpadding=\"12\" "
        "style=\"border: 1px solid #64748B; background-color: #F8FAFC;\">"
        "<tr><td style=\"color: #1E293B;\">%1</td></tr></table><p></p>")
                             .arg(tr("Text Box — Type your highlighted note or executive summary here."));
    m_editor->textCursor().insertHtml(html);
}

void WriterInsertTabHandler::onInsertTextArt()
{
    if (!m_editor) {
        return;
    }

    bool ok = false;
    const QString bannerTitle = QInputDialog::getText(
        m_editor, tr("Insert Styled Text Art"), tr("Enter decorative heading text:"),
        QLineEdit::Normal, m_doc.title, &ok).trimmed();

    if (!ok || bannerTitle.isEmpty()) {
        return;
    }

    const QString html = QStringLiteral(
        "<p style=\"font-size:22pt; font-weight:bold; color:#1D4ED8; letter-spacing:1px;\">%1</p>")
                             .arg(bannerTitle.toHtmlEscaped());
    m_editor->textCursor().insertHtml(html);
}

void WriterInsertTabHandler::onInsertDateTime()
{
    if (!m_editor) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QStringList formats = {
        now.toString(QStringLiteral("dd/MM/yyyy")),
        now.toString(QStringLiteral("yyyy-MM-dd")),
        now.toString(QStringLiteral("dd/MM/yyyy HH:mm")),
        now.toString(Qt::ISODate)
    };

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        m_editor, tr("Insert Date & Time"), tr("Select format:"), formats, 0, false, &ok);
    if (ok && !chosen.isEmpty()) {
        m_editor->textCursor().insertText(chosen);
    }
}

void WriterInsertTabHandler::onInsertTextFromFile()
{
    if (!m_editor) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        m_editor,
        tr("Insert Text from File"),
        QString(),
        tr("Text & Markup Files (*.txt *.md *.html *.htm *.csv);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(m_editor, tr("File Error"), tr("Could not read the selected file."));
        return;
    }

    QTextStream in(&file);
    QtCompat::setUtf8Encoding(in);
    const QString content = in.readAll();

    if (filePath.endsWith(QStringLiteral(".html"), Qt::CaseInsensitive)
        || filePath.endsWith(QStringLiteral(".htm"), Qt::CaseInsensitive)) {
        m_editor->textCursor().insertHtml(content);
    } else {
        m_editor->textCursor().insertText(content);
    }
}

void WriterInsertTabHandler::onInsertField()
{
    if (!m_editor) {
        return;
    }

    const BootstrapConfig boot = BootstrapManager::loadConfig();
    const QString text = m_editor->toPlainText().trimmed();
    const int words = text.isEmpty() ? 0 : text.split(QRegularExpression(QStringLiteral("\\s+")), QtCompat::SkipEmptyParts).size();

    const QStringList fields = {
        tr("Document Author: %1").arg(m_doc.ownerName),
        tr("Current Editor: %1").arg(m_session.fullName),
        tr("Official Version: v%1").arg(m_doc.currentVersion),
        tr("Workstation ID: %1").arg(boot.hostLabel.isEmpty() ? QStringLiteral("PC-001") : boot.hostLabel),
        tr("Word Count: %1").arg(words)
    };

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        m_editor, tr("Insert Dynamic Document Field"), tr("Select field:"), fields, 0, false, &ok);
    if (ok && !chosen.isEmpty()) {
        m_editor->textCursor().insertHtml(
            QStringLiteral("<span style=\"background-color:#E2E8F0; color:#0F172A;\">%1</span>").arg(chosen.toHtmlEscaped()));
    }
}

void WriterInsertTabHandler::onInsertEquation()
{
    if (!m_editor) {
        return;
    }

    const QStringList presets = {
        QStringLiteral("E = mc²"),
        QStringLiteral("x = (-b ± √(b² - 4ac)) / 2a"),
        QStringLiteral("∑ (xᵢ - μ)² / N"),
        QStringLiteral("∫ f(x) dx = F(b) - F(a)"),
        QStringLiteral("Δ% = ((V₂ - V₁) / V₁) × 100")
    };

    bool ok = false;
    const QString eq = QInputDialog::getItem(
        m_editor, tr("Insert Equation"), tr("Select or edit mathematical formula:"), presets, 0, true, &ok).trimmed();
    if (ok && !eq.isEmpty()) {
        m_editor->textCursor().insertHtml(
            QStringLiteral("<span style=\"font-family:'Liberation Serif','Times New Roman',serif; font-style:italic; font-size:12pt;\">%1</span>")
                .arg(eq.toHtmlEscaped()));
    }
}

void WriterInsertTabHandler::onInsertSymbol()
{
    if (!m_editor) {
        return;
    }

    QDialog dlg(m_editor);
    dlg.setWindowTitle(tr("Insert Special Symbol"));

    QVBoxLayout *vbox = new QVBoxLayout(&dlg);
    QGridLayout *grid = new QGridLayout();

    const QStringList symbols = {
        QStringLiteral("©"), QStringLiteral("®"), QStringLiteral("™"), QStringLiteral("§"),
        QStringLiteral("¶"), QStringLiteral("•"), QStringLiteral("†"), QStringLiteral("‡"),
        QStringLiteral("€"), QStringLiteral("£"), QStringLiteral("¥"), QStringLiteral("¢"),
        QStringLiteral("±"), QStringLiteral("×"), QStringLiteral("÷"), QStringLiteral("≠"),
        QStringLiteral("≤"), QStringLiteral("≥"), QStringLiteral("≈"), QStringLiteral("∞"),
        QStringLiteral("√"), QStringLiteral("∑"), QStringLiteral("∏"), QStringLiteral("π"),
        QStringLiteral("α"), QStringLiteral("β"), QStringLiteral("Δ"), QStringLiteral("Ω"),
        QStringLiteral("←"), QStringLiteral("→"), QStringLiteral("↑"), QStringLiteral("↓")
    };

    int row = 0;
    int col = 0;
    for (const QString &sym : symbols) {
        QPushButton *btn = new QPushButton(sym, &dlg);
        btn->setFixedSize(36, 32);
        grid->addWidget(btn, row, col);
        connect(btn, &QPushButton::clicked, &dlg, [&, sym]() {
            m_editor->textCursor().insertText(sym);
            dlg.accept();
        });
        if (++col >= 8) {
            col = 0;
            ++row;
        }
    }

    vbox->addLayout(grid);
    dlg.exec();
}

void WriterInsertTabHandler::onInsertContentControl()
{
    if (!m_editor) {
        return;
    }

    const QStringList controls = {
        tr("Check Box [ ☐ ]"),
        tr("Checked Box [ ☑ ]"),
        tr("Dropdown Placeholder [ Select Option ▼ ]"),
        tr("Signature Line [ ______________________ ]")
    };

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        m_editor, tr("Insert Content Control"), tr("Select interactive form control:"), controls, 0, false, &ok);
    if (!ok || chosen.isEmpty()) {
        return;
    }

    QString snippet;
    if (chosen == controls.at(0)) {
        snippet = QStringLiteral("☐ ");
    } else if (chosen == controls.at(1)) {
        snippet = QStringLiteral("☑ ");
    } else if (chosen == controls.at(2)) {
        snippet = QStringLiteral("<span style=\"background-color:#F1F5F9; border:1px solid #94A3B8;\">[ %1 ▼ ]</span> ")
                      .arg(tr("Choose an item"));
    } else {
        snippet = QStringLiteral("<br><span style=\"color:#334155;\">______________________________<br>%1</span><br>")
                      .arg(tr("Authorized Signature"));
    }

    m_editor->textCursor().insertHtml(snippet);
}
