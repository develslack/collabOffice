#ifndef WRITERDRAWTABHANDLER_H
#define WRITERDRAWTABHANDLER_H

#include <QObject>
#include <QList>
#include <QColor>
#include <QIcon>
#include "IWorkspaceModule.h"

class QTextEdit;
class QAction;
class QActionGroup;
class QMenu;
class PageInkOverlayWidget;

class WriterDrawTabHandler : public QObject
{
    Q_OBJECT

public:
    explicit WriterDrawTabHandler(QTextEdit *editor,
                                  bool readOnlyMode,
                                  QObject *parent = nullptr);

    QList<CommandGroupSpec> commandGroups() const;
    void retranslateUi();

    bool hasPendingInk() const;
    void commitInkToDocument();

signals:
    void drawingCommitted();

private slots:
    void onSelectToolTriggered();
    void onGreenPenTriggered();
    void onRedPenTriggered();
    void onHighlighterTriggered();
    void onEraserTriggered();
    void onStampInkTriggered();
    void onClearInkTriggered();

private:
    void createActions();
    QMenu *buildToolOptionsMenu(QColor &targetColor, qreal &targetWidth, QAction *ownerAction, bool isHighlighter);

    static QIcon makeSelectIcon();
    static QIcon makePenIcon(const QColor &barColor, bool isHighlighter);
    static QIcon makeEraserIcon();

    QTextEdit *m_editor;
    PageInkOverlayWidget *m_overlay;
    bool m_readOnly;

    QColor m_greenPenColor;
    qreal m_greenPenWidth;

    QColor m_redPenColor;
    qreal m_redPenWidth;

    QColor m_highlighterColor;
    qreal m_highlighterWidth;

    QActionGroup *m_toolGroup;
    QAction *m_actSelect;
    QAction *m_actGreenPen;
    QAction *m_actRedPen;
    QAction *m_actHighlighter;
    QAction *m_actEraser;
    QAction *m_actStampToDoc;
    QAction *m_actClearInk;
};

#endif // WRITERDRAWTABHANDLER_H
