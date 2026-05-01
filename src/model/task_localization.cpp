#include "task_localization.h"

namespace vc::model {

TaskLocalization::TaskLocalization(QString name, QString id, QObject* parent)
    : ITask(id, parent), m_config() {

    this->setName(name);

    this->blockSignals(true);
    this->setTaskConfig(&m_config);
    this->blockSignals(false);

    m_patternManager = new mtc::PatternGroupManager(this);
}

void TaskLocalization::setupTask() {

}

void TaskLocalization::executeLocalization() {

}

void TaskLocalization::onCameraNumberChanged() {

}

void TaskLocalization::onPatternNumberChanged() {

}


} // namespace vc::model
