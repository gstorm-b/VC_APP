#ifndef TASK_LOCALIZATION_H
#define TASK_LOCALIZATION_H

#include "model/itask.h"
#include "device/camera/camera_device.h"
#include "model/camera_worker.h"
#include "device/plc/mc_protocol_device.h"
#include "task_localization_config.h"
#include "matching/image_matcher.h"

#include "matching/pattern_group_manager.h"
#include <opencv2/imgcodecs.hpp>

namespace vc::model {

class TaskLocalization : public ITask {
    Q_OBJECT

public:
    explicit TaskLocalization(QString name, QString id = "", QObject* parent = nullptr);

    TaskType taskType()  const override {
        return TaskType::LocalizationTask;
    }

    bool isValid() const override {
        return m_isValid;
    }

    void setTaskLocalizeConfig(TaskLocalizeConfig &cfg) {
        m_config = cfg;
        this->setTaskConfig(&m_config);
    }

    TaskLocalizeConfig taskLocalizeConfig() const {
        return m_config;
    }

    mtc::PatternGroupManager* patternManager() const {
        return m_patternManager;
    }

    virtual QMap<QString, cv::Mat> getTaskImageMap() override {
        QMap<QString, cv::Mat> map;

        // here to save image into database

        // cv::Mat  mat_test_1 = cv::imread("C:/DGB/Project/ncr_picking/build/test_1.png");
        // cv::Mat  mat_test_2 = cv::imread("C:/DGB/Project/ncr_picking/build/test_2.bmp");
        // cv::Mat  mat_test_3 = cv::imread("C:/DGB/Project/ncr_picking/build/test_3.bmp");

        // map.insert("Test_1", mat_test_1);
        // map.insert("Test_2", mat_test_2);
        // map.insert("Test_3", mat_test_3);

        // qDebug() << "Test save to blob";
        // qDebug() << map.keys();

        return map;
    }

    virtual bool loadTaskImageMap(QMap<QString, cv::Mat> &mapping) override  {

        // here to load image into database

        // qDebug() << "Test load from blob";
        // qDebug() << mapping.keys();

        // auto map_it = mapping.cbegin();
        // while (map_it != mapping.cend()) {
        //     cv::Mat test_mat = map_it.value();

        //     QString file_name = QString("%1.bmp").arg(map_it.key());

        //     cv::imwrite(file_name.toStdString(), test_mat);

        //     map_it++;
        // }

        return false;
    }


public slots:
    void setupTask();
    void executeLocalization();

private:
    void onCameraNumberChanged();
    void onPatternNumberChanged();

signals:
    void startPLCRequest();
    void startCameraRequest();

private:
    bool m_isValid{false};

    TaskLocalizeConfig m_config;

    vc::runtime::CameraWorker *m_worker{nullptr};
    std::shared_ptr<device::CameraDevice> m_camera;
    std::shared_ptr<device::McProtocolDevice> m_mc_protocol;

    mtc::ImageMatcher m_matcher;
    mtc::PatternGroupManager *m_patternManager;
    // mtc::MatchGroup *m_match_model;
};


}


#endif // TASK_LOCALIZATION_H
