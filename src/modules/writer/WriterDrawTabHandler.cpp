#include "WriterDrawTabHandler.h"
#include "PageInkOverlayWidget.h"

#include <QTextEdit>
#include <QTextCursor>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QColorDialog>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QBuffer>
#include <QByteArray>
#include <QApplication>
#include <QStyle>

WriterDrawTabHandler::WriterDrawTabHandler(QTextEdit *editor,
                                           bool readOnlyMode,
                                           QObject *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_overlay(nullptr)
    , m_readOnly(readOnlyMode)
    , m_greenPenColor(QColor(0x22, 0xC5, 0x5E))
    , m_greenPenWidth(2.5)
    , m_redPenColor(QColor(0xEF, 0x44, 0x44))
    , m_redPenWidth(2.5)
    , m_highlighterColor(QColor(0xFA, 0xCC, 0x15))
    , m_highlighterWidth(14.0)
    , m_toolGroup(new QActionGroup(this))
{
    if (m_editor) {
        m_overlay = new PageInkOverlayWidget(m_editor);
    }

    createActions();
    retranslateUi();
}

void WriterDrawTabHandler::createActions()
{
    QStyle *st = QApplication::style();
    m_toolGroup->setExclusive(true);

    m_actSelect = new QAction(makeSelectIcon(), QString(), this);
    m_actSelect->setCheckable(true);
    m_actSelect->setChecked(true);
    m_toolGroup->addAction(m_actSelect);

    m_actGreenPen = new QAction(makePenIcon(m_greenPenColor, false), QString(), this);
    m_actGreenPen->setCheckable(true);
    m_toolGroup->addAction(m_actGreenPen);

    m_actRedPen = new QAction(makePenIcon(m_redPenColor, false), QString(), this);
    m_actRedPen->setCheckable(true);
    m_toolGroup->addAction(m_actRedPen);

    m_actHighlighter = new QAction(makePenIcon(m_highlighterColor, true), QString(), this);
    m_actHighlighter->setCheckable(true);
    m_toolGroup->addAction(m_actHighlighter);

    m_actEraser = new QAction(makeEraserIcon(), QString(), this);
    m_actEraser->setCheckable(true);
    m_toolGroup->addAction(m_actEraser);

    m_actStampToDoc = new QAction(st->standardIcon(QStyle::SP_DialogApplyButton), QString(), this);
    m_actClearInk = new QAction(st->standardIcon(QStyle::SP_DialogResetButton), QString(), this);

    // Attach dropdown menus (thickness & color) to pens and highlighter just like ONLYOFFICE
    m_actGreenPen->setMenu(buildToolOptionsMenu(m_greenPenColor, m_greenPenWidth, m_actGreenPen, false));
    m_actRedPen->setMenu(buildToolOptionsMenu(m_redPenColor, m_redPenWidth, m_actRedPen, false));
    m_actHighlighter->setMenu(buildToolOptionsMenu(m_highlighterColor, m_highlighterWidth, m_actHighlighter, true));

    const bool editable = !m_readOnly;
    for (QAction *act : { m_actGreenPen, m_actRedPen, m_actHighlighter, m_actEraser, m_actStampToDoc, m_actClearInk }) {
        act->setEnabled(editable);
    }

    connect(m_actSelect, &QAction::triggered, this, &WriterDrawTabHandler::onSelectToolTriggered);
    connect(m_actGreenPen, &QAction::triggered, this, &WriterDrawTabHandler::onGreenPenTriggered);
    connect(m_actRedPen, &QAction::triggered, this, &WriterDrawTabHandler::onRedPenTriggered);
    connect(m_actHighlighter, &QAction::triggered, this, &WriterDrawTabHandler::onHighlighterTriggered);
    connect(m_actEraser, &QAction::triggered, this, &WriterDrawTabHandler::onEraserTriggered);
    connect(m_actStampToDoc, &QAction::triggered, this, &WriterDrawTabHandler::onStampInkTriggered);
    connect(m_actClearInk, &QAction::triggered, this, &WriterDrawTabHandler::onClearInkTriggered);
}

QMenu *WriterDrawTabHandler::buildToolOptionsMenu(QColor &targetColor, qreal &targetWidth,
                                                  QAction *ownerAction, bool isHighlighter)
{
    QMenu *menu = new QMenu();
    const QList<qreal> widths = isHighlighter
                                    ? QList<qreal>{ 8.0, 14.0, 20.0, 28.0 }
                                    : QList<qreal>{ 1.5, 2.5, 4.0, 6.0 };

    for (qreal w : widths) {
        QAction *wAct = menu->addAction(tr("Thickness: %1 px").arg(w));
        connect(wAct, &QAction::triggered, this, [this, &targetWidth, w, ownerAction]() {
            targetWidth = w;
            ownerAction->trigger();
        });
    }

    menu->addSeparator();
    QAction *colorAct = menu->addAction(tr("Custom Ink Color..."));
    connect(colorAct, &QAction::triggered, this, [this, &targetColor, ownerAction, isHighlighter]() {
        const QColor chosen = QColorDialog::getColor(targetColor, m_editor, tr("Select Ink Color"));
        if (chosen.isValid()) {
            targetColor = chosen;
            ownerAction->setIcon(makePenIcon(targetColor, isHighlighter));
            ownerAction->trigger();
        }
    });

    return menu;
}

QList<CommandGroupSpec> WriterDrawTabHandler::commandGroups() const
{
    const QString tabName = tr("Draw");

    CommandGroupSpec toolsGrp;
    toolsGrp.tabTitle = tabName;
    toolsGrp.groupTitle = tr("Drawing Tools");
    toolsGrp.actions = { m_actSelect, m_actGreenPen, m_actRedPen, m_actHighlighter, m_actEraser };

    CommandGroupSpec persistGrp;
    persistGrp.tabTitle = tabName;
    persistGrp.groupTitle = tr("Page Ink");
    persistGrp.actions = { m_actStampToDoc, m_actClearInk };

    return { toolsGrp, persistGrp };
}

void WriterDrawTabHandler::retranslateUi()
{
    m_actSelect->setText(tr("Select"));
    m_actGreenPen->setText(tr("Pen 1"));
    m_actRedPen->setText(tr("Pen 2"));
    m_actHighlighter->setText(tr("Highlighter"));
    m_actEraser->setText(tr("Eraser"));
    m_actStampToDoc->setText(tr("Stamp to Page"));
    m_actClearInk->setText(tr("Clear Ink"));
}

bool WriterDrawTabHandler::hasPendingInk() const
{
    return m_overlay && m_overlay->hasStrokes();
}

void WriterDrawTabHandler::commitInkToDocument()
{
    if (!m_editor || !m_overlay || !m_overlay->hasStrokes()) {
        return;
    }

    const QImage cropped = m_overlay->renderCroppedInkImage();
    if (cropped.isNull()) {
        return;
    }

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    cropped.save(&buffer, "PNG");

    const QString html = QStringLiteral("<p><img src=\"data:image/png;base64,%1\" width=\"%2\" height=\"%3\" /></p>")
                             .arg(QString::fromLatin1(pngBytes.toBase64()))
                             .arg(cropped.width())
                             .arg(cropped.height());

    m_editor->textCursor().insertHtml(html);
    m_overlay->clearAllStrokes();

    // Return to normal text cursor mode after stamping
    m_actSelect->setChecked(true);
    m_overlay->setToolMode(PageInkOverlayWidget::ToolMode::Select);

    emit drawingCommitted();
}

void WriterDrawTabHandler::onSelectToolTriggered()
{
    if (m_overlay) {
        m_overlay->setToolMode(PageInkOverlayWidget::ToolMode::Select);
    }
}

void WriterDrawTabHandler::onGreenPenTriggered()
{
    if (m_overlay) {
        m_overlay->setStrokeColor(m_greenPenColor);
        m_overlay->setStrokeWidth(m_greenPenWidth);
        m_overlay->setToolMode(PageInkOverlayWidget::ToolMode::Pen);
    }
}

void WriterDrawTabHandler::onRedPenTriggered()
{
    if (m_overlay) {
        m_overlay->setStrokeColor(m_redPenColor);
        m_overlay->setStrokeWidth(m_redPenWidth);
        m_overlay->setToolMode(PageInkOverlayWidget::ToolMode::Pen);
    }
}

void WriterDrawTabHandler::onHighlighterTriggered()
{
    if (m_overlay) {
        m_overlay->setStrokeColor(m_highlighterColor);
        m_overlay->setStrokeWidth(m_highlighterWidth);
        m_overlay->setToolMode(PageInkOverlayWidget::ToolMode::Highlighter);
    }
}

void WriterDrawTabHandler::onEraserTriggered()
{
    if (m_overlay) {
        m_overlay->setToolMode(PageInkOverlayWidget::ToolMode::Eraser);
    }
}

void WriterDrawTabHandler::onStampInkTriggered()
{
    commitInkToDocument();
}

void WriterDrawTabHandler::onClearInkTriggered()
{
    if (m_overlay) {
        m_overlay->clearAllStrokes();
    }
}

QIcon WriterDrawTabHandler::makeSelectIcon()
{
    QPixmap pm(28, 28);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(0xE4, 0xE4, 0xE7), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPolygonF arrow;
    arrow << QPointF(8, 5) << QPointF(8, 21) << QPointF(12.5, 17)
          << QPointF(15.5, 23) << QPointF(18, 21.8) << QPointF(15, 15.8)
          << QPointF(20.5, 15.8);
    p.drawPolygon(arrow);
    return QIcon(pm);
}

QIcon WriterDrawTabHandler::makePenIcon(const QColor &barColor, bool isHighlighter)
{
    QPixmap pm(28, 28);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(0xE4, 0xE4, 0xE7), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    if (isHighlighter) {
        QPolygonF body;
        body << QPointF(16, 4) << QPointF(21, 9) << QPointF(12, 18) << QPointF(7, 13);
        p.drawPolygon(body);
        p.drawLine(QPointF(7, 13), QPointF(6, 19));
        p.drawLine(QPointF(12, 18), QPointF(6, 19));
    } else {
        QPolygonF nib;
        nib << QPointF(17, 4) << QPointF(21, 8) << QPointF(11, 18) << QPointF(7, 19) << QPointF(8, 15);
        p.drawPolygon(nib);
    }

    // Colored ink indicator bar at the bottom (matching ONLYOFFICE Draw icons)
    p.setPen(Qt::NoPen);
    p.setBrush(barColor);
    p.drawRoundedRect(QRectF(4, 22, 20, 4), 1.0, 1.0);

    return QIcon(pm);
}

QIcon WriterDrawTabHandler::makeEraserIcon()
{
    QPixmap pm(28, 28);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(0xE4, 0xE4, 0xE7), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPolygonF block;
    block << QPointF(15, 5) << QPointF(22, 12) << QPointF(13, 21) << QPointF(6, 14);
    p.drawPolygon(block);
    p.drawLine(QPointF(10.5, 9.5), QPointF(17.5, 16.5));
    p.drawLine(QPointF(6, 22), QPointF(22, 22));

    return QIcon(pm);
}
