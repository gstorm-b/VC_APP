#ifndef PATTERN_CANVAS_H
#define PATTERN_CANVAS_H

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QPixmap>
#include <opencv2/core.hpp>

// ─────────────────────────────────────────────────────────────────────────────
//  AddPatternImageCanvas
//
//  Shared canvas widget used by both AddPatternWizard and EditPatternWizard.
//  One widget, switchable mode:
//
//      None   — just shows the image.
//      Crop   — draggable crop rect with corner handles + rule-of-thirds grid.
//      Pick   — click to set pick point; renders crosshair + ring.
//      Box    — read-only render of symmetric jaw pair using current pick + box
//               configuration.  No interaction; settings come from the parent.
//      Finish — render pick crosshair AND symmetric jaw pair (review).
//
//  Coordinates are in display pixels of the canvas (560×380).  The host is
//  responsible for mapping these to real image coordinates if needed.
// ─────────────────────────────────────────────────────────────────────────────

class AddPatternImageCanvas : public QWidget {
    Q_OBJECT
public:
    enum Mode { None, Crop, Pick, Box, Finish };

    explicit AddPatternImageCanvas(QWidget *parent = nullptr);

    // Image source
    void setImage(const QPixmap &pix);
    void setImage(const cv::Mat &mat);
    void clearImage();
    bool hasImage() const { return !m_pix.isNull(); }

    // Mode
    void setMode(Mode m);
    Mode mode() const { return m_mode; }

    // Crop
    void setCrop(const QRect &r);
    QRect crop() const { return m_crop; }

    // Pick
    void setPick(const QPoint &p);
    QPoint pick() const { return m_pick; }

    // Box (rendered only)
    void setBoxConfig(double w, double h, double dist, double angleDeg);

    // Locked overlay (Edit wizard step 1 — purple scrim + lock badge)
    void setLocked(bool locked);

signals:
    void cropChanged(const QRect &r);
    void pickChanged(const QPoint &p);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    enum CornerHandle { None_=-1, TL=0, TR=1, BL=2, BR=3 };

    CornerHandle hitCorner(const QPoint &p) const;

    Mode    m_mode{None};
    QPixmap m_pix;          // currently displayed image
    QRect   m_crop{80, 60, 400, 260};
    QPoint  m_pick{280, 195};

    double  m_boxW{120}, m_boxH{80};
    double  m_boxDist{90}, m_boxAngle{0};

    bool    m_locked{false};

    // crop drag state
    bool         m_dragging{false};
    CornerHandle m_dragCorner{None_};
    QPoint       m_dragStart;
    QRect        m_dragStartCrop;
};

#endif // PATTERN_CANVAS_H
