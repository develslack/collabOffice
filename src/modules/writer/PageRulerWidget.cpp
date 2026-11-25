#include "PageRulerWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <cmath>

PageRulerWidget::PageRulerWidget(Orientation orientation, QWidget *parent)
    : QWidget(parent)
    , m_orientation(orientation)
    , m_zoomFactor(1.0)
    , m_marginStartPx(56)
    , m_marginEndPx(56)
{
    if (m_orientation == Horizontal) {
        setFixedHeight(22);
    } else {
        setFixedWidth(22);
    }
}

void PageRulerWidget::setZoomFactor(double zoom)
{
    m_zoomFactor = qMax(0.25, zoom);
    update();
}

void PageRulerWidget::setMarginsPx(int marginStartPx, int marginEndPx)
{
    m_marginStartPx = marginStartPx;
    m_marginEndPx = marginEndPx;
    update();
}

void PageRulerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const QColor marginBg(0x2D, 0x2D, 0x31);
    const QColor activeBg(0x3B, 0x3B, 0x40);
    const QColor tickColor(0x9C, 0xA3, 0xAF);
    const QColor textColor(0xE4, 0xE4, 0xE7);
    const QColor borderColor(0x4B, 0x55, 0x63);

    p.fillRect(rect(), marginBg);

    const int length = (m_orientation == Horizontal) ? width() : height();
    const int activeStart = qBound(0, m_marginStartPx, length);
    const int activeEnd = qBound(activeStart, length - m_marginEndPx, length);

    if (m_orientation == Horizontal) {
        p.fillRect(QRect(activeStart, 2, activeEnd - activeStart, height() - 4), activeBg);
        p.setPen(borderColor);
        p.drawLine(0, height() - 1, width(), height() - 1);
    } else {
        p.fillRect(QRect(2, activeStart, width() - 4, activeEnd - activeStart), activeBg);
        p.setPen(borderColor);
        p.drawLine(width() - 1, 0, width() - 1, height());
    }

    QFont f = font();
    f.setPointSize(7);
    p.setFont(f);

    // 1 cm at 96 DPI ~= 37.795 px
    const double pxPerCm = 37.795 * m_zoomFactor;
    const double pxPerQuarterCm = pxPerCm / 4.0;
    const int totalSteps = static_cast<int>(length / pxPerQuarterCm);

    for (int step = 0; step <= totalSteps; ++step) {
        const int pos = static_cast<int>(std::round(step * pxPerQuarterCm));
        const bool isFullCm = (step % 4 == 0);
        const bool isHalfCm = (step % 2 == 0) && !isFullCm;

        p.setPen(tickColor);
        if (m_orientation == Horizontal) {
            const int tickLen = isFullCm ? 9 : (isHalfCm ? 6 : 3);
            p.drawLine(pos, height() - tickLen, pos, height() - 2);

            if (isFullCm && step > 0) {
                const int cmNumber = step / 4;
                p.setPen(textColor);
                p.drawText(QRect(pos - 12, 1, 24, 12), Qt::AlignCenter, QString::number(cmNumber));
            }
        } else {
            const int tickLen = isFullCm ? 9 : (isHalfCm ? 6 : 3);
            p.drawLine(width() - tickLen, pos, width() - 2, pos);

            if (isFullCm && step > 0) {
                const int cmNumber = step / 4;
                p.setPen(textColor);
                p.drawText(QRect(1, pos - 8, width() - 6, 16), Qt::AlignCenter, QString::number(cmNumber));
            }
        }
    }
}
