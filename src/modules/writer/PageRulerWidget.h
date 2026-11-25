#ifndef PAGERULERWIDGET_H
#define PAGERULERWIDGET_H

#include <QWidget>

class PageRulerWidget : public QWidget
{
    Q_OBJECT

public:
    enum Orientation {
        Horizontal,
        Vertical
    };

    explicit PageRulerWidget(Orientation orientation, QWidget *parent = nullptr);

    void setZoomFactor(double zoom);
    void setMarginsPx(int marginStartPx, int marginEndPx);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Orientation m_orientation;
    double m_zoomFactor;
    int m_marginStartPx;
    int m_marginEndPx;
};

#endif // PAGERULERWIDGET_H
