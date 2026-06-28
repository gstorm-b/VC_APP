#ifndef MATCH_OBJECT_H
#define MATCH_OBJECT_H

// #include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"
#include "match_pattern.h"

namespace mtc {

class ImageMatcher;

class MatchedObject {
public:
    struct GripperBox {
        cv::RotatedRect box_left;
        cv::RotatedRect box_right;
        std::vector<cv::Point2f> box_left_pts;
        std::vector<cv::Point2f> box_right_pts;
        cv::Point2f center;
        double distance;
    };

    MatchedObject() :
        matched_Angle(0.0),
        matched_Score(0.0),
        pattern_name(L""),
        pattern_index(-1),
        m_parent(nullptr),
        m_hasCollision(false),
        m_isOutsideConditionRoi(false) {

    }

    void setParent(MatchPattern *parent) {
        this->m_parent = parent;
    }

    const MatchPattern* parent() const {
        return m_parent;
    }

    void translateByTopLeft(cv::Point2f topleft) {
        point_LT = topleft + point_LT;
        point_RT = topleft + point_RT;
        point_LB = topleft + point_LB;
        point_RB = topleft + point_RB;
        point_Center = topleft + point_Center;
    }

    void computeMatchAngle(double angle) {
        matched_Angle = angle;
        if (matched_Angle < -180) {
            matched_Angle += 360;
        }
        if (matched_Angle > 180) {
            matched_Angle -= 360;
        }
    }

    void computePointAngle(double initial_angle) {
        point_angle = matched_Angle + initial_angle;
        if (point_angle < -180) {
            point_angle += 360;
        }
        if (point_angle > 180) {
            point_angle -= 360;
        }
    }

    void fillObjectIntoImage(cv::Mat &image, cv::Point2f topLeft, double angle) {
        cv::Mat mask = m_parent->getMaskImage();
        if (mask.empty()) {
            return;
        }
        this->drawMaskIntoImage(image, mask, cv::Scalar(0, 0, 255));
    }

    void drawMaskPatternIntoImage(cv::Mat &image) {
        cv::Mat mask = m_parent->getPatternMask();
        // m_raw_picking_point = m_parent->pickPosition;
        this->drawMaskIntoImage(image, mask, cv::Scalar(255, 0, 0));
    }

    void drawGripperBoxToImage(cv::Mat &image, cv::Scalar color) {
        for (int i=0; i<4; i++) {
            cv::line(image, m_gripperBox.box_right_pts[i],
                     m_gripperBox.box_right_pts[(i+1)%4], color, 1);
        }

        for (int i=0; i<4; i++) {
            cv::line(image, m_gripperBox.box_left_pts[i],
                     m_gripperBox.box_left_pts[(i+1)%4], color, 1);
        }
    }

    void computeGripperBox(cv::Size box_size, double distance, double angle) {
        double rad = (point_angle + angle) * CV_PI / 180.0f;
        cv::Point2f offset(distance * cos(rad), distance * sin(rad));
        m_gripperBox.box_right = cv::RotatedRect(point_Center + offset, box_size, point_angle + angle);
        m_gripperBox.box_left = cv::RotatedRect(point_Center - offset, box_size, point_angle + angle);
        m_gripperBox.box_right.points(m_gripperBox.box_right_pts);
        m_gripperBox.box_left.points(m_gripperBox.box_left_pts);
    }

    bool checkCollisionObject(std::vector<std::vector<cv::Point>> &contours, std::vector<int> &indexes) {
        m_hasCollision = false;
        for(int idx=0;idx<indexes.size();idx++) {
            std::vector<cv::Point>& con = contours.at(indexes[idx]);
            for(int con_idx=0;con_idx<con.size();con_idx++) {
                if(cv::pointPolygonTest(m_gripperBox.box_left_pts, con.at(con_idx), false) >= 0) {
                    m_hasCollision = true;
                    goto _ret_point;
                }

                if(cv::pointPolygonTest(m_gripperBox.box_right_pts, con.at(con_idx), false) >= 0) {
                    m_hasCollision = true;
                    goto _ret_point;
                }
            }
        }

        _ret_point:
        return m_hasCollision;
    }

    bool hasCollision() const {
        return m_hasCollision;
    }

    bool outSideConditionRoiCheck(const cv::Point2f& tl, const cv::Point2f& br) {
        float left   = (tl.x < br.x) ? tl.x : br.x;
        float top    = (tl.y < br.y) ? tl.y : br.y;
        float right  = (tl.x > br.x) ? tl.x : br.x;
        float bottom = (tl.y > br.y) ? tl.y : br.y;
        m_isOutsideConditionRoi = (point_Center.x >= left && point_Center.x <= right && point_Center.y >= top && point_Center.y <= bottom);
        m_isOutsideConditionRoi = !m_isOutsideConditionRoi;
        return m_isOutsideConditionRoi;
    }

    bool isOutsideConditionRoi() const {
        return m_isOutsideConditionRoi;
    }

    bool isPossibleToPick() const {
        return m_isPossibleToPicking;
    }

    void setPossibleToPick(bool state) {
        m_isPossibleToPicking = state;
    }

    const GripperBox& gripperBox() const {
        return m_gripperBox;
    }

private:
    void drawMaskIntoImage(cv::Mat &image, cv::Mat &mask, cv::Scalar color) {
        if (mask.empty() || image.empty()) {
            return;
        }

        // wrap always counter clockwise
        double angle = -matched_Angle;

        // rotate mask
        cv::Mat rotMat = cv::getRotationMatrix2D(cv::Point2f(0,0), angle, 1.0);

        // get mask bounding box
        std::vector<cv::Point2f> corners = {
            cv::Point2f(0,0),
            cv::Point2f(mask.cols,0),
            cv::Point2f(mask.cols, mask.rows),
            cv::Point2f(0,mask.rows)
        };

        // transform rotated box
        std::vector<cv::Point2f> rotatedCorners;
        cv::transform(corners, rotatedCorners, rotMat);

        cv::Rect2f bbox = cv::boundingRect(rotatedCorners);

        rotMat.at<double>(0,2) -= bbox.x;
        rotMat.at<double>(1,2) -= bbox.y;

        cv::Mat rotatedMask;
        cv::warpAffine(mask, rotatedMask, rotMat, bbox.size(),
                       cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));

        // get raw bouding box
        std::vector<cv::Point2f> raw_corners = {
            cv::Point2f(point_LT),
            cv::Point2f(point_LB),
            cv::Point2f(point_RT),
            cv::Point2f(point_RB)
        };
        cv::Rect raw_bouding = cv::boundingRect(raw_corners);

        cv::Rect roi(raw_bouding.x, raw_bouding.y, rotatedMask.cols, rotatedMask.rows);
        roi = roi & cv::Rect(0, 0, image.cols, image.rows);

        cv::Mat roiImg = image(roi);
        cv::Point mask_new_tf((raw_bouding.x < 0) ? -raw_bouding.x : 0,
                              (raw_bouding.y < 0) ? -raw_bouding.y : 0);
        cv::Mat maskROI = rotatedMask(cv::Rect(mask_new_tf.x, mask_new_tf.y,roi.width,roi.height));
        roiImg.setTo(color, maskROI);

        // cv::circle(image, point_LT, 2, cv::Scalar(255, 0, 0), 2);
        // cv::Point new_pts(raw_bouding.x + rotatedCorners[0].x + rotMat.at<double>(0,2),
        //                   raw_bouding.y + rotatedCorners[0].y + rotMat.at<double>(1,2));
        // cv::circle(image, new_pts, 2, cv::Scalar(0, 255, 0), 2);
    }

    friend class mtc::ImageMatcher;

public:
    cv::Point2f point_LT;
    cv::Point2f point_RT;
    cv::Point2f point_LB;
    cv::Point2f point_RB;
    cv::Point2f point_Center;
    cv::Point3f point_offset;
    cv::RotatedRect pickingBox;
    double point_angle;
    double matched_Angle;
    double matched_Score;
    std::wstring pattern_name;
    int pattern_index;
    cv::Mat image;

private:
    MatchPattern *m_parent{nullptr};
    GripperBox m_gripperBox;
    bool m_hasCollision{false};
    bool m_isPossibleToPicking{false};
    bool m_isOutsideConditionRoi{false};
};

}

#endif // MATCH_OBJECT_H
