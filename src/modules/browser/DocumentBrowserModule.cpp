#include "DocumentBrowserModule.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QApplication>
#include <QStyle>

DocumentBrowserModule::DocumentBrowserModule(const UserSession &session, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_rootWidget(nullptr)
    , m_editSearch(nullptr)
    , m_tblDocuments(nullptr)
    , m_btnOpen(nullptr)
    , m_btnShare(nullptr)
    , m_actRefresh(nullptr)
    , m_actNewDoc(nullptr)
    , m_actNewSheet(nullptr)
    , m_actOpenDoc(nullptr)
    , m_actShareDoc(nullptr)
{
    buildUi();
    refreshDocumentList();
}

QString DocumentBrowserModule::moduleId() const
{
    return QStringLiteral("document_browser");
}

QString DocumentBrowserModule::displayTitle() const
{
    return tr("Documents");
}

QIcon DocumentBrowserModule::moduleIcon() const
{
    return QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
}

QWidget *DocumentBrowserModule::centralWidget()
{
    return m_rootWidget;
}

QList<CommandGroupSpec> DocumentBrowserModule::commandGroups() const
{
    CommandGroupSpec g1;
    g1.tabTitle = tr("Repository");
    g1.groupTitle = tr("Documents");
    g1.actions = { m_actNewDoc, m_actNewSheet, m_actOpenDoc, m_actShareDoc, m_actRefresh };
    return { g1 };
}

QString DocumentBrowserModule::statusSummary() const
{
    if (m_session.isAdmin()) {
        return tr("Administrator View: Showing all %1 documents in the organization").arg(m_cachedDocs.size());
    }
    return tr("My Workspace: Showing %1 documents where you have ownership or assigned permissions").arg(m_cachedDocs.size());
}

void DocumentBrowserModule::retranslateUi()
{
    if (m_actRefresh) m_actRefresh->setText(tr("Refresh"));
    if (m_actNewDoc) m_actNewDoc->setText(tr("New Text Doc"));
    if (m_actNewSheet) m_actNewSheet->setText(tr("New Spreadsheet"));
    if (m_actOpenDoc) m_actOpenDoc->setText(tr("Open Selected"));
    if (m_actShareDoc) m_actShareDoc->setText(tr("Permissions"));
    if (m_editSearch) m_editSearch->setPlaceholderText(tr("Filter visible documents by title or owner..."));
    if (m_btnOpen) m_btnOpen->setText(tr("Open Document"));
    if (m_btnShare) m_btnShare->setText(tr("Manage Access"));

    if (m_tblDocuments) {
        m_tblDocuments->setHorizontalHeaderLabels({
            tr("Title"), tr("Type"), tr("Owner"), tr("Version"), tr("Your Access"), tr("Last Updated")
        });
    }
    refreshDocumentList();
}

void DocumentBrowserModule::buildUi()
{
    QStyle *st = QApplication::style();
    m_actNewDoc = new QAction(st->standardIcon(QStyle::SP_FileIcon), tr("New Text Doc"), this);
    m_actNewSheet = new QAction(st->standardIcon(QStyle::SP_FileDialogDetailedView), tr("New Spreadsheet"), this);
    m_actOpenDoc = new QAction(st->standardIcon(QStyle::SP_DialogOpenButton), tr("Open Selected"), this);
    m_actShareDoc = new QAction(st->standardIcon(QStyle::SP_DialogApplyButton), tr("Permissions"), this);
    m_actRefresh = new QAction(st->standardIcon(QStyle::SP_BrowserReload), tr("Refresh"), this);

    m_rootWidget = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(m_rootWidget);

    QHBoxLayout *topBar = new QHBoxLayout();
    m_editSearch = new QLineEdit(m_rootWidget);
    m_editSearch->setPlaceholderText(tr("Filter visible documents by title or owner..."));
    m_btnOpen = new QPushButton(tr("Open Document"), m_rootWidget);
    m_btnShare = new QPushButton(tr("Manage Access"), m_rootWidget);

    topBar->addWidget(m_editSearch, 1);
    topBar->addWidget(m_btnOpen);
    topBar->addWidget(m_btnShare);
    vbox->addLayout(topBar);

    m_tblDocuments = new QTableWidget(0, 6, m_rootWidget);
    m_tblDocuments->setHorizontalHeaderLabels({
        tr("Title"), tr("Type"), tr("Owner"), tr("Version"), tr("Your Access"), tr("Last Updated")
    });
    m_tblDocuments->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tblDocuments->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tblDocuments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tblDocuments->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    vbox->addWidget(m_tblDocuments, 1);

    connect(m_actNewDoc, &QAction::triggered, this, &DocumentBrowserModule::createNewTextDocumentRequested);
    connect(m_actNewSheet, &QAction::triggered, this, &DocumentBrowserModule::createNewSpreadsheetRequested);
    connect(m_actOpenDoc, &QAction::triggered, this, &DocumentBrowserModule::onOpenSelectedClicked);
    connect(m_actShareDoc, &QAction::triggered, this, &DocumentBrowserModule::onShareSelectedClicked);
    connect(m_actRefresh, &QAction::triggered, this, &DocumentBrowserModule::refreshDocumentList);
    connect(m_btnOpen, &QPushButton::clicked, this, &DocumentBrowserModule::onOpenSelectedClicked);
    connect(m_btnShare, &QPushButton::clicked, this, &DocumentBrowserModule::onShareSelectedClicked);
    connect(m_editSearch, &QLineEdit::textChanged, this, &DocumentBrowserModule::onSearchFilterChanged);
    connect(m_tblDocuments, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        onOpenSelectedClicked();
    });
}

QString DocumentBrowserModule::localizedAccessLabel(const QString &accessCode) const
{
    if (accessCode == QStringLiteral("OWNER")) return tr("Owner (Full Control)");
    if (accessCode == QStringLiteral("ADMIN_VIEW")) return tr("Administrator (Global Audit)");
    if (accessCode == QStringLiteral("CO_ADMIN")) return tr("Co-Admin");
    if (accessCode == QStringLiteral("PROPOSE_EDIT")) return tr("Contributor (Virtual Draft)");
    return tr("Read Only");
}

void DocumentBrowserModule::refreshDocumentList()
{
    m_cachedDocs = DatabaseManager::instance().listAccessibleDocuments(m_session);
    onSearchFilterChanged(m_editSearch ? m_editSearch->text() : QString());
}

void DocumentBrowserModule::onSearchFilterChanged(const QString &text)
{
    const QString filter = text.trimmed().toLower();
    m_tblDocuments->setRowCount(0);

    for (int i = 0; i < m_cachedDocs.size(); ++i) {
        const DocumentSummary &doc = m_cachedDocs.at(i);
        if (!filter.isEmpty()
            && !doc.title.toLower().contains(filter)
            && !doc.ownerName.toLower().contains(filter)) {
            continue;
        }

        const int row = m_tblDocuments->rowCount();
        m_tblDocuments->insertRow(row);

        QTableWidgetItem *titleItem = new QTableWidgetItem(doc.title);
        titleItem->setData(Qt::UserRole, i); // Index into m_cachedDocs

        const QString typeText = (doc.docType == QStringLiteral("SPREADSHEET"))
                                     ? tr("Spreadsheet (.xls)")
                                     : tr("Text Document (.doc)");

        m_tblDocuments->setItem(row, 0, titleItem);
        m_tblDocuments->setItem(row, 1, new QTableWidgetItem(typeText));
        m_tblDocuments->setItem(row, 2, new QTableWidgetItem(doc.ownerName));
        m_tblDocuments->setItem(row, 3, new QTableWidgetItem(QStringLiteral("v%1").arg(doc.currentVersion)));
        m_tblDocuments->setItem(row, 4, new QTableWidgetItem(localizedAccessLabel(doc.effectiveAccess)));
        m_tblDocuments->setItem(row, 5, new QTableWidgetItem(doc.updatedAt));
    }
}

void DocumentBrowserModule::onOpenSelectedClicked()
{
    const int row = m_tblDocuments->currentRow();
    if (row < 0) {
        return;
    }
    const int docIdx = m_tblDocuments->item(row, 0)->data(Qt::UserRole).toInt();
    if (docIdx >= 0 && docIdx < m_cachedDocs.size()) {
        emit openDocumentRequested(m_cachedDocs.at(docIdx));
    }
}

void DocumentBrowserModule::onShareSelectedClicked()
{
    const int row = m_tblDocuments->currentRow();
    if (row < 0) {
        return;
    }
    const int docIdx = m_tblDocuments->item(row, 0)->data(Qt::UserRole).toInt();
    if (docIdx < 0 || docIdx >= m_cachedDocs.size()) {
        return;
    }

    const DocumentSummary &doc = m_cachedDocs.at(docIdx);
    if (doc.ownerId != m_session.userId && !m_session.isAdmin()) {
        QMessageBox::warning(m_rootWidget, tr("Access Denied"),
                             tr("Only the document owner or the System Administrator can modify document permissions."));
        return;
    }

    QDialog dlg(m_rootWidget);
    dlg.setWindowTitle(tr("Permissions - %1").arg(doc.title));
    dlg.setMinimumWidth(420);

    QFormLayout *form = new QFormLayout(&dlg);
    QComboBox *comboTargetType = new QComboBox(&dlg);
    comboTargetType->addItem(tr("User"), QStringLiteral("USER"));
    comboTargetType->addItem(tr("Group"), QStringLiteral("GROUP"));

    QComboBox *comboEntity = new QComboBox(&dlg);
    QComboBox *comboAccess = new QComboBox(&dlg);
    comboAccess->addItem(tr("Contributor (Virtual Draft / Propose Changes)"), QStringLiteral("PROPOSE_EDIT"));
    comboAccess->addItem(tr("Read Only"), QStringLiteral("READ"));
    comboAccess->addItem(tr("Co-Admin"), QStringLiteral("CO_ADMIN"));
    comboAccess->addItem(tr("Revoke Access (NONE)"), QStringLiteral("NONE"));

    auto populateEntities = [&]() {
        comboEntity->clear();
        if (comboTargetType->currentData().toString() == QStringLiteral("USER")) {
            for (const UserRecord &u : DatabaseManager::instance().listAllUsers()) {
                if (u.id != doc.ownerId) {
                    comboEntity->addItem(QStringLiteral("%1 (%2)").arg(u.fullName, u.username), u.id);
                }
            }
        } else {
            for (const GroupRecord &g : DatabaseManager::instance().listAllGroups()) {
                comboEntity->addItem(g.name, g.id);
            }
        }
    };
    populateEntities();
    connect(comboTargetType, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, populateEntities);

    QPushButton *btnGrant = new QPushButton(tr("Apply Permission"), &dlg);
    form->addRow(tr("Assign To:"), comboTargetType);
    form->addRow(tr("User / Group:"), comboEntity);
    form->addRow(tr("Permission Level:"), comboAccess);
    form->addRow(btnGrant);

    connect(btnGrant, &QPushButton::clicked, &dlg, [&]() {
        if (comboEntity->count() == 0) {
            return;
        }
        const bool isUser = (comboTargetType->currentData().toString() == QStringLiteral("USER"));
        const QString targetId = comboEntity->currentData().toString();
        const QString level = comboAccess->currentData().toString();
        QString err;

        if (DatabaseManager::instance().grantDocumentPermission(
                doc.id,
                isUser ? targetId : QString(),
                isUser ? QString() : targetId,
                level,
                m_session.userId,
                err)) {
            QMessageBox::information(&dlg, tr("Permissions Updated"), tr("Document permissions saved successfully."));
            dlg.accept();
        } else {
            QMessageBox::critical(&dlg, tr("Error"), err);
        }
    });

    dlg.exec();
}
