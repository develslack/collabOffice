#ifndef WRITERINSERTTABHANDLER_H
#define WRITERINSERTTABHANDLER_H

#include <QObject>
#include <QList>
#include "IWorkspaceModule.h"
#include "DatabaseManager.h"

class QTextEdit;
class QAction;

class WriterInsertTabHandler : public QObject
{
    Q_OBJECT

public:
    explicit WriterInsertTabHandler(QTextEdit *editor,
                                    const DocumentSummary &docSummary,
                                    const UserSession &session,
                                    bool readOnlyMode,
                                    QObject *parent = nullptr);

    QList<CommandGroupSpec> commandGroups() const;
    void retranslateUi();
    void setEditor(QTextEdit *editor);

private slots:
    // Pages
    void onInsertBlankPage();
    void onInsertPageBreak();

    // Tables
    void onInsertTable();

    // Illustrations
    void onInsertImage();
    void onInsertShape();
    void onInsertDiagram();

    // Comments & Links
    void onInsertComment();
    void onInsertLink();

    // Header & Footer
    void onInsertHeaderFooter();
    void onInsertPageNumber();

    // Text objects
    void onInsertTextBox();
    void onInsertTextArt();
    void onInsertDateTime();
    void onInsertTextFromFile();
    void onInsertField();

    // Symbols & Content Controls
    void onInsertEquation();
    void onInsertSymbol();
    void onInsertContentControl();

private:
    void createActions();

    QTextEdit *m_editor;
    DocumentSummary m_doc;
    UserSession m_session;
    bool m_readOnly;

    QAction *m_actBlankPage;
    QAction *m_actPageBreak;
    QAction *m_actTable;
    QAction *m_actImage;
    QAction *m_actShape;
    QAction *m_actDiagram;
    QAction *m_actComment;
    QAction *m_actLink;
    QAction *m_actHeaderFooter;
    QAction *m_actPageNumber;
    QAction *m_actTextBox;
    QAction *m_actTextArt;
    QAction *m_actDateTime;
    QAction *m_actTextFromFile;
    QAction *m_actField;
    QAction *m_actEquation;
    QAction *m_actSymbol;
    QAction *m_actContentControl;
};

#endif // WRITERINSERTTABHANDLER_H
