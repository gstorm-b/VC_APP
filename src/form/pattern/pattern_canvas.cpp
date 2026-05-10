#include "pattern_canvas.h"
#include "pattern_theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtMath>
#include <opencv2/imgproc.hpp>

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────

AddPatternImageCanvas::AddPatternImageCanvas(QWidget *parent) : QWidget(parent) {
    setMinimumSize(400, 280);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
}

// ── Image setters ───────────────────────────────────────────────────────────

void AddPatternImageCanvas::setImage(const QPixmap &pix) {
    m_pix = pix;
    update();
}

void AddPatternImageCanvas::setImage(const cv::Mat &mat) {
    if (mat.empty()) { m_pix = QPixmap(); update(); return; }

    QImage img;
    if (mat.type() == CV_8UC1) {
        img = QImage(mat.data, mat.cols, mat.rows,
                     static_cast<int>(mat.step),
                     QImage::Format_Grayscale8).copy();
    } else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        img = QImage(rgb.data, rgb.cols, rgb.rows,
                     static_cast<int>(rgb.step),
                     QImage::Format_RGB888).copy();
    } else if (mat.type() == CV_8UC4) {
        cv::Mat rgba;
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
        img = QImage(rgba.data, rgba.cols, rgba.rows,
                     static_cast<int>(rgba.step),
                     QImage::Format_RGBA8888).copy();
    }
    m_pix = QPixmap::fromImage(img);
    update();
}

void AddPatternImageCanvas::clearImage() { m_pix = QPixmap(); update(); }

void AddPatternImageCanvas::setMode(Mode m)            { m_mode = m; update(); }
void AddPatternImageCanvas::setCrop(const QRect &r)    { m_crop = r; update(); }
void AddPatternImageCanvas::setPick(const QPoint &p)   { m_pick = p; update(); }
void AddPatternImageCanvas::setBoxConfig(double w, double h, double d, double a) {
    m_boxW = w; m_boxH = h; m_boxDist = d; m_boxAngle = a;
    update();
}
void AddPatternImageCanvas::setLocked(bool locked)     { m_locked = locked; update(); }

// ─────────────────────────────────────────────────────────────────────────────
//  Painting
// ─────────────────────────────────────────────────────────────────────────────

void AddPatternImageCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Background — checker grid placeholder when no image
    p.fillRect(rect(), QColor("#181818"));

    if (m_pix.isNull()) {
        // Subtle grid
        p.setPen(QPen(QColor(ptn::BD), 0.5));
        for (int x = 0; x < width(); x += 20)  p.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 20) p.drawLine(0, y, width(), y);

        p.setPen(QColor(ptn::TXT3));
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(rect(), Qt::AlignCenter, tr("No image"));
    } else {
        // Fit image into widget while preserving aspect
        QPixmap scaled = m_pix.scaled(size(), Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        const int x = (width()  - scaled.width())  / 2;
        const int y = (height() - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
    }

    // ── Mode-specific overlay ───────────────────────────────────────────────
    if (m_mode == Crop) {
        // Darken outside crop
        const QColor scrim(0, 0, 0, 170);
        QPainterPath outer; outer.addRect(rect());
        QPainterPath inner; inner.addRect(m_crop);
        p.setBrush(scrim); p.setPen(Qt::NoPen);
        p.drawPath(outer.subtracted(inner));

        // Crop rect
        p.setPen(QPen(QColor(ptn::ACC), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRect(m_crop);

        // Rule-of-thirds inside
        QPen tp;
        tp.setColor(QColor(ptn::ACC));
        tp.setWidthF(1);
        QColor tc(ptn::ACC); tc.setAlpha(85);
        p.setPen(QPen(tc, 1));
        p.drawLine(m_crop.left() + m_crop.width()/3,    m_crop.top(),
                   m_crop.left() + m_crop.width()/3,    m_crop.bottom());
        p.drawLine(m_crop.left() + 2*m_crop.width()/3,  m_crop.top(),
                   m_crop.left() + 2*m_crop.width()/3,  m_crop.bottom());
        p.drawLine(m_crop.left(),  m_crop.top()    + m_crop.height()/3,
                   m_crop.right(), m_crop.top()    + m_crop.height()/3);
        p.drawLine(m_crop.left(),  m_crop.top()    + 2*m_crop.height()/3,
                   m_crop.right(), m_crop.top()    + 2*m_crop.height()/3);

        // Corner handles
        const QList<QPoint> corners = {
            m_crop.topLeft(), m_crop.topRight(),
            m_crop.bottomLeft(), m_crop.bottomRight()
        };
        p.setBrush(QColor(ptn::ACC));
        p.setPen(QPen(QColor("white"), 1.5));
        for (const QPoint &c : corners) {
            p.drawRect(QRect(c.x() - 5, c.y() - 5, 11, 11));
        }
    } else if (m_mode == Pick) {
        // Crosshair lines
        QColor warn(ptn::WARN);
        QColor warn2(ptn::WARN); warn2.setAlpha(204);
        p.setPen(QPen(warn2, 1));
        p.drawLine(0, m_pick.y(), width(), m_pick.y());
        p.drawLine(m_pick.x(), 0, m_pick.x(), height());

        // Ring
        QColor ringFill(ptn::WARN); ringFill.setAlpha(34);
        p.setBrush(ringFill);
        p.setPen(QPen(warn, 1.5));
        p.drawEllipse(QPointF(m_pick), 9, 9);

        // Label
        p.setPen(warn);
        p.setFont(QFont("JetBrains Mono", 8, QFont::Bold));
        p.drawText(m_pick.x() + 12, m_pick.y() - 12,
                   QString("PICK (%1, %2)").arg(m_pick.x()).arg(m_pick.y()));
    } else if (m_mode == Box || m_mode == Finish) {
        // Render symmetric jaw pair around pick point
        const QColor jawColor(ptn::OK);
        QColor jawFill(ptn::OK); jawFill.setAlpha(34);

        auto drawBox = [&](double angleDeg, const QString &label) {
            const double r = qDegreesToRadians(angleDeg);
            const double cx = m_pick.x() + std::cos(r) * m_boxDist;
            const double cy = m_pick.y() + std::sin(r) * m_boxDist;

            // Connector
            p.setPen(QPen(jawColor, 1, Qt::DashLine));
            p.drawLine(QPointF(m_pick), QPointF(cx, cy));

            // Rect rotated by angleDeg around (cx, cy)
            p.save();
            p.translate(cx, cy);
            p.rotate(angleDeg);
            p.setBrush(jawFill);
            p.setPen(QPen(jawColor, 1.5));
            p.drawRect(QRectF(-m_boxW/2, -m_boxH/2, m_boxW, m_boxH));
            p.setBrush(jawColor); p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(0, 0), 2.5, 2.5);
            p.restore();

            // Label
            p.setPen(jawColor);
            p.setFont(QFont("JetBrains Mono", 8, QFont::Bold));
            p.drawText(QPointF(cx - 4, cy - m_boxH/2 - 4), label);
        };
        drawBox(m_boxAngle,        "A");
        drawBox(m_boxAngle + 180,  "B");

        // Pick crosshair on top
        p.setPen(QPen(QColor("white"), 1));
        p.drawLine(m_pick.x() - 12, m_pick.y(), m_pick.x() + 12, m_pick.y());
        p.drawLine(m_pick.x(), m_pick.y() - 12, m_pick.x(), m_pick.y() + 12);
        p.setBrush(QColor(ptn::WARN)); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(m_pick), 3, 3);
    }

    // ── Locked scrim (Edit wizard step 1) ───────────────────────────────────
    if (m_locked) {
        QColor scrim(ptn::LOCK); scrim.setAlpha(80);
        p.fillRect(rect(), scrim);

        // Lock badge top-right
        const int bw = 86, bh = 22;
        QRect badge(width() - bw - 12, 12, bw, bh);
        QColor badgeBg(ptn::LOCK);
        p.setBrush(badgeBg);
        p.setPen(QPen(QColor("white"), 1));
        p.drawRoundedRect(badge, 4, 4);
        p.setPen(QColor("white"));
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.drawText(badge, Qt::AlignCenter, "🔒  LOCKED");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse interaction
// ─────────────────────────────────────────────────────────────────────────────

AddPatternImageCanvas::CornerHandle
AddPatternImageCanvas::hitCorner(const QPoint &p) const {
    const int slack = 10;
    const QList<QPoint> c = {
        m_crop.topLeft(), m_crop.topRight(),
        m_crop.bottomLeft(), m_crop.bottomRight()
    };
    for (int i = 0; i < 4; ++i) {
        if (qAbs(p.x() - c[i].x()) <= slack &&
            qAbs(p.y() - c[i].y()) <= slack)
            return CornerHandle(i);
    }
    return None_;
}

void AddPatternImageCanvas::mousePressEvent(QMouseEvent *e) {
    if (m_locked) return;

    if (m_mode == Crop && e->button() == Qt::LeftButton) {
        CornerHandle c = hitCorner(e->pos());
        if (c != None_) {
            m_dragging      = true;
            m_dragCorner    = c;
            m_dragStart     = e->pos();
            m_dragStartCrop = m_crop;
        }
    } else if (m_mode == Pick && e->button() == Qt::LeftButton) {
        m_pick = e->pos();
        update();
        emit pickChanged(m_pick);
    }
}

void AddPatternImageCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (m_locked) return;

    if (m_mode == Crop && m_dragging) {
        const int dx = e->pos().x() - m_dragStart.x();
        const int dy = e->pos().y() - m_dragStart.y();
        QRect r = m_dragStartCrop;

        switch (m_dragCorner) {
        case TL: r.setTopLeft(r.topLeft()         + QPoint(dx, dy)); break;
        case TR: r.setTopRight(r.topRight()       + QPoint(dx, dy)); break;
        case BL: r.setBottomLeft(r.bottomLeft()   + QPoint(dx, dy)); break;
        case BR: r.setBottomRight(r.bottomRight() + QPoint(dx, dy)); break;
        default: break;
        }

        // Clamp and enforce minimum size
        if (r.width()  < 40) r.setWidth(40);
        if (r.height() < 40) r.setHeight(40);
        r = r.intersected(rect());

        m_crop = r;
        update();
        emit cropChanged(m_crop);
    } else if (m_mode == Crop) {
        // Hover cursor
        CornerHandle c = hitCorner(e->pos());
        switch (c) {
        case TL: case BR: setCursor(Qt::SizeFDiagCursor); break;
        case TR: case BL: setCursor(Qt::SizeBDiagCursor); break;
        default:          setCursor(Qt::ArrowCursor);     break;
        }
    } else if (m_mode == Pick) {
        setCursor(Qt::CrossCursor);
    }
}

void AddPatternImageCanvas::mouseReleaseEvent(QMouseEvent *e) {
    Q_UNUSED(e);
    m_dragging   = false;
    m_dragCorner = None_;
}
