#ifndef PAGEINKOVERLAYWIDGET_H
#define PAGEINKOVERLAYWIDGET_H

#include <QWidget>
#include <QPainterPath>
#include <QColor>
#include <QList>
#include <QImage>

class QTextEdit;

class PageInkOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    enum class ToolMode {
        Select,
        Pen,
        Highlighter,
        Eraser
    };
    Q_ENUM(ToolMode)

    explicit PageInkOverlayWidget(QTextEdit *targetEditor);

    void setToolMode(ToolMode mode);
    ToolMode toolMode() const;

    void setStrokeColor(const QColor &color);
    void setStrokeWidth(qreal widthPx);

    bool hasStrokes() const;
    void clearAllStrokes();

    // Renders drawn ink strokes cropped to their bounding box as a transparent PNG
    QImage renderCroppedInkImage() const;

signals:
    void inkModified();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct InkStroke {
        QPainterPath path;
        QColor color;
        qreal width = 2.5;
        bool isHighlighter = false;
    };

    void eraseAtPoint(const QPointF &pt);

    QTextEdit *m_editor;
    ToolMode m_toolMode;
    QColor m_strokeColor;
    qreal m_strokeWidth;
    bool m_isDrawing;

    QList<InkStroke> m_strokes;
    InkStroke m_currentStroke;
};

#endif // PAGEINKOVERLAYWIDGET_H
