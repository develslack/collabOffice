#include "PageInkOverlayWidget.h"
#include "QtCompat.h"

#include <QTextEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEvent>

PageInkOverlayWidget::PageInkOverlayWidget(QTextEdit *targetEditor)
    : QWidget(targetEditor ? targetEditor->viewport() : nullptr)
    , m_editor(targetEditor)
    , m_toolMode(ToolMode::Select)
    , m_strokeColor(QColor(0x16, 0xA3, 0x4A))
    , m_strokeWidth(2.5)
    , m_isDrawing(false)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    if (m_editor && m_editor->viewport()) {
        QWidget *vp = m_editor->viewport();
        setGeometry(vp->rect());
        vp->installEventFilter(this);
        raise();
        show();
    }
}

void PageInkOverlayWidget::setToolMode(ToolMode mode)
{
    m_toolMode = mode;
    const bool passThrough = (m_toolMode == ToolMode::Select);
    setAttribute(Qt::WA_TransparentForMouseEvents, passThrough);

    if (passThrough) {
        unsetCursor();
    } else {
        setCursor(Qt::CrossCursor);
        raise();
    }
}

PageInkOverlayWidget::ToolMode PageInkOverlayWidget::toolMode() const
{
    return m_toolMode;
}

void PageInkOverlayWidget::setStrokeColor(const QColor &color)
{
    if (color.isValid()) {
        m_strokeColor = color;
    }
}

void PageInkOverlayWidget::setStrokeWidth(qreal widthPx)
{
    m_strokeWidth = qBound(1.0, widthPx, 36.0);
}

bool PageInkOverlayWidget::hasStrokes() const
{
    return !m_strokes.isEmpty();
}

void PageInkOverlayWidget::clearAllStrokes()
{
    m_strokes.clear();
    m_isDrawing = false;
    update();
}

QImage PageInkOverlayWidget::renderCroppedInkImage() const
{
    if (m_strokes.isEmpty()) {
        return QImage();
    }

    QRectF combinedBounds;
    for (const InkStroke &s : m_strokes) {
        const qreal pad = s.width + 4.0;
        const QRectF strokeRect = s.path.boundingRect().adjusted(-pad, -pad, pad, pad);
        combinedBounds = combinedBounds.isNull() ? strokeRect : combinedBounds.united(strokeRect);
    }

    const QRect pixelRect = combinedBounds.toAlignedRect().intersected(rect());
    if (pixelRect.width() <= 2 || pixelRect.height() <= 2) {
        return QImage();
    }

    QImage img(pixelRect.size(), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(-pixelRect.topLeft());

    for (const InkStroke &s : m_strokes) {
        QColor c = s.color;
        if (s.isHighlighter) {
            c.setAlpha(110);
        }
        QPen pen(c, s.width, Qt::SolidLine,
                 s.isHighlighter ? Qt::FlatCap : Qt::RoundCap,
                 Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawPath(s.path);
    }

    return img;
}

bool PageInkOverlayWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (m_editor && watched == m_editor->viewport() && event->type() == QEvent::Resize) {
        setGeometry(m_editor->viewport()->rect());
    }
    return QWidget::eventFilter(watched, event);
}

void PageInkOverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_strokes.isEmpty() && !m_isDrawing) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto drawSingleStroke = [&painter](const InkStroke &s) {
        QColor c = s.color;
        if (s.isHighlighter) {
            c.setAlpha(110);
        }
        QPen pen(c, s.width, Qt::SolidLine,
                 s.isHighlighter ? Qt::FlatCap : Qt::RoundCap,
                 Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawPath(s.path);
    };

    for (const InkStroke &s : m_strokes) {
        drawSingleStroke(s);
    }
    if (m_isDrawing) {
        drawSingleStroke(m_currentStroke);
    }
}

void PageInkOverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_toolMode == ToolMode::Select || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    const QPointF pos = QtCompat::mouseLocalPos(event);

    if (m_toolMode == ToolMode::Eraser) {
        m_isDrawing = true;
        eraseAtPoint(pos);
        event->accept();
        return;
    }

    m_isDrawing = true;
    m_currentStroke = InkStroke();
    m_currentStroke.path = QPainterPath(pos);
    m_currentStroke.path.lineTo(pos + QPointF(0.1, 0.1)); // Ensure single-click dots render
    m_currentStroke.color = m_strokeColor;
    m_currentStroke.width = m_strokeWidth;
    m_currentStroke.isHighlighter = (m_toolMode == ToolMode::Highlighter);

    update();
    event->accept();
}

void PageInkOverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_isDrawing || m_toolMode == ToolMode::Select) {
        event->ignore();
        return;
    }

    const QPointF pos = QtCompat::mouseLocalPos(event);

    if (m_toolMode == ToolMode::Eraser) {
        eraseAtPoint(pos);
        event->accept();
        return;
    }

    m_currentStroke.path.lineTo(pos);
    update();
    event->accept();
}

void PageInkOverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_isDrawing || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    m_isDrawing = false;
    if (m_toolMode == ToolMode::Pen || m_toolMode == ToolMode::Highlighter) {
        m_strokes.append(m_currentStroke);
        emit inkModified();
    }

    update();
    event->accept();
}

void PageInkOverlayWidget::eraseAtPoint(const QPointF &pt)
{
    const QRectF eraserBox(pt.x() - 10.0, pt.y() - 10.0, 20.0, 20.0);
    bool removedAny = false;

    for (int i = m_strokes.size() - 1; i >= 0; --i) {
        if (m_strokes.at(i).path.intersects(eraserBox)) {
            m_strokes.removeAt(i);
            removedAny = true;
        }
    }

    if (removedAny) {
        emit inkModified();
        update();
    }
}
