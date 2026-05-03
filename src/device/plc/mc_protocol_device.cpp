#include "mc_protocol_device.h"
#include "mc_msg_tcp_client.h"
#include "mc_request.h"

#include <memory>
#include <QThread>

namespace vc::device {
McProtocolDevice::McProtocolDevice(QString id, QString name, QObject* parent)
    : IDevice(id, name, parent) {

    IDevice::setDeviceConfig(&m_config);
}

McProtocolDevice::~McProtocolDevice() {
    // Destructor call from another thread
}

void McProtocolDevice::deviceTerminate() {
    LOG_DEV_DEBUG << "MC protocol device terminate, device name" << name()
                  << ", id" << id();
    if (isDeviceConnected()) {
        LOG_DEV_DEBUG << "MC protocol disconnect, destroy timer";
        deviceDisconnect();
    }
}

bool McProtocolDevice::deviceConnect() {
    QMutexLocker locker(&m_mutex);

    if (!this->initialize_mc_device()) {
        return false;
    }

    McMsgInterface::MsgIfState msg_port_state = m_msg_interface->ConnectToPort();
    if (msg_port_state != McMsgInterface::MsgIfState::Connected) {
        LOG_DEV_INFO << "connect error occurred," << m_msg_interface->GetErrorDescription();
        this->setConnectionStatus(ConnectStatus::ConnectFailed, m_msg_interface->GetErrorDescription());
        return false;
    }

    LOG_DEV_INFO << "MC Device connected";
    this->setConnectionStatus(ConnectStatus::Connected);
    m_polling_timer->start();
    return true;
}

bool McProtocolDevice::deviceDisconnect() {
    QMutexLocker locker(&m_mutex);

    m_msg_interface->DisconnectFromPort();
    LOG_DEV_INFO << "MC Device disconnected";
    this->setConnectionStatus(ConnectStatus::Disconnected);

    m_msg_interface.release();
    m_frame.release();
    if (m_polling_timer->isActive()) {
        m_polling_timer->stop();
    }
    m_polling_timer->deleteLater();
    return false;
}

void McProtocolDevice::setDeviceConfig(IDeviceCfg *cfg) {
    if (!cfg) {
        return;
    }

    if (this->isDeviceConnected()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    McProtocolConfig *mc_config = static_cast<McProtocolConfig*>(cfg);
    m_config.setContext(mc_config->context());
    optimizeDeviceMap();
    // change and emit signal
    IDevice::setDeviceConfig(&m_config);
}

void McProtocolDevice::setMcProtocolConfig(McProtocolConfig& cfg) {
    if (this->isDeviceConnected()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_config.setContext(cfg.context());
    optimizeDeviceMap();
    // change and emit signal
    IDevice::setDeviceConfig(&m_config);
}

McProtocolConfig McProtocolDevice::mcProtocolConfig() const {
    return m_config;
}

QStringList McProtocolDevice::getAvailableBits() {
    return m_m_device_names;
}

QStringList McProtocolDevice::getAvailableWords() {
    return m_d_device_names;
}

bool McProtocolDevice::pushRequest(IRequest *request) {
    if (request->type() != RequestType::Request_MC) {
        return false;
    }

    m_mutex.lock();
    std::shared_ptr<MCRequest> mc_request = std::static_pointer_cast<MCRequest>(request->clone());
    mc_request->m_value.detach();
    m_request_queue.push_back(mc_request);
    m_mutex.unlock();
    return true;
}

bool McProtocolDevice::fromJson(const QJsonObject &obj) {
    QMutexLocker locker(&m_mutex);
    bool state = IDevice::fromJson(obj);
    optimizeDeviceMap();
    return state;
}

void McProtocolDevice::onPollingTimerTimeOut() {
    if (!this->isDeviceConnected()) {
        if(m_polling_timer) {
            if (m_polling_timer->isActive()) {
                m_polling_timer->stop();
            }
        }
        return;
    }

    polling_query();
}

void McProtocolDevice::onMsgInterfaceReadReady() {
    response_handle();
}

void McProtocolDevice::onSetCommActiveDevice() {
    std::shared_ptr<MCRequest> request = std::make_shared<MCRequest>(
        MCRequest::WriteBit, 'M', m_comm_active_m_device, 1);
    request->buildWriteData_Bit_Device((m_comm_active_m_device_value) ? 0x01 : 0x00);
    m_request_queue.push_back(request);

    m_comm_active_m_device_value = !m_comm_active_m_device_value;
}

bool McProtocolDevice::initialize_mc_device() {
    McFrameType frame_type = m_config.context()->frameType();
    McMsgItfType msg_type = m_config.context()->msgConfig()->type();

    // setup frame builder
    switch (frame_type) {
    case vc::device::mc::McFrameType::Frame_3E:
        m_config.context()->setDeviceMap(&m_device_map);
        m_frame = std::make_unique<Frame3E>();
        LOG_DEV_INFO << "Initialize MC Protocol Frame 3E.";
        break;
    default:
        return false;
    }

    // setup msg interface
    switch (msg_type) {
    case vc::device::mc::McMsgItfType::EthernetTCPIP:
        m_msg_interface = std::make_unique<McEthernetTcpPort>();
        m_msg_interface->SetConfig(m_config.context()->msgConfig());
        LOG_DEV_INFO << "Initialize Ethernet TCP/IP message interface.";
        break;
    default:
        return false;
    }
    connect(m_msg_interface->ioDevice(), &QIODevice::readyRead,
            this, &McProtocolDevice::onMsgInterfaceReadReady);


    McContext *ctx = m_config.context();
    // setup polling timer
    m_polling_timer = new QTimer(this);
    m_polling_timer->setTimerType(Qt::PreciseTimer);
    m_polling_timer->setInterval(ctx->m_refreshInterval);
    LOG_DEV_INFO << "MC Protocol Device refresh interval:" << ctx->m_refreshInterval;
    connect(m_polling_timer, &QTimer::timeout,
            this, &McProtocolDevice::onPollingTimerTimeOut);

    // setup polling device map
    optimizeDeviceMap();

    // setup state flag
    m_wait_for_response = false;
    is_first_time_polling = true;
    m_update_command_index = 0;
    m_retry_by_timeout = 0;
    m_retry_count = 0;

    m_current_request.reset();
    m_request_queue.clear();
    m_read_buffer.clear();

    m_comm_active_m_device = m_config.context()->m_activeMDevice;

    return true;
}

void McProtocolDevice::polling_query() {
    if (m_wait_for_response) {
        std::chrono::high_resolution_clock::time_point current_time_point
            = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration time_lasp =
            std::chrono::duration_cast<std::chrono::milliseconds>(current_time_point - m_sent_time_point);
        if (time_lasp.count() > m_config.context()->msgConfig()->m_responseTimeout) {
            // response timeout handle;
            retry_request_handle();
        }
        return;
    }

    m_mutex.lock();
    if (!m_request_queue.empty()) {
        m_current_request = m_request_queue.takeFirst();
        m_mutex.unlock();
    } else {
        m_mutex.unlock();

        if (m_polling_request_queue.isEmpty()) {
            return;
        }

        m_current_request = m_polling_request_queue.at(m_update_command_index++);
        if (m_update_command_index >= m_polling_request_queue.count()) {
            m_update_command_index = 0;
            is_first_time_polling = false;
            emit pollingUpdate(m_device_map);

            onSetCommActiveDevice();
        }
    }

    request_handle();
}

void McProtocolDevice::request_handle() {
    if (m_current_request != nullptr) {
        QByteArray send_frame;

        m_mutex.lock();
        MCFrameAbstract::FrameReturnCode rt_code = m_frame->makeSendFrame(m_current_request.get(), m_config.context(), send_frame);
        m_mutex.unlock();

        if (rt_code == MCFrameAbstract::RequestFrameOK) {
            McMsgInterface::MsgErrorState send_state = m_msg_interface->SendMsg(send_frame);

            if (send_state != McMsgInterface::MsgErrorState::NoError) {
                LOG_USER_ERR << "error while send frame";
                m_current_request.reset();
                return;
            }

            m_wait_for_response = true;
            m_sent_time_point = std::chrono::high_resolution_clock::now();
            // LOG_USER_INFO << "Send frame to PLC:" << send_frame.toHex(' ');
        } else {
            m_current_request.reset();
            LOG_USER_ERR << "make request frame error";
        }
    }
}

void McProtocolDevice::response_handle() {
    if (!m_wait_for_response) {
        return;
    }

    if (m_current_request == nullptr) {
        return;
    }

    QByteArray receive_bytes;
    McMsgInterface::MsgErrorState recieve_state = m_msg_interface->ReceiveMsg(receive_bytes, 1);
    // OLOG_INFO << "Received frame from PLC:" << receive_bytes.toHex(' ');

    if (recieve_state != McMsgInterface::MsgErrorState::NoError) {
        if (recieve_state == McMsgInterface::MsgErrorState::BufferEmpty) {
            LOG_USER_ERR << "empty frame";
            return;
        }
        LOG_USER_ERR << "error while receive frame, error code" << recieve_state;
        return;
    }

    m_mutex.lock();
    MCFrameAbstract::FrameReturnCode rt_code = m_frame->parseReceiveFrame(m_current_request.get(), m_config.context(), receive_bytes);
    m_mutex.unlock();

    if (rt_code == MCFrameAbstract::ResponseOk) {
        // OLOG_INFO << "reponse frame status error," << m_frame->lastErrorDescription();
        check_device_changed();

    } else if  (rt_code == MCFrameAbstract::ResponseError) {
        LOG_USER_ERR << "reponse frame status error," << m_frame->lastErrorDescription();
    } else if  (rt_code == MCFrameAbstract::ResponseInvalid) {
        LOG_USER_ERR << "reponse frame invalid," << m_frame->lastErrorDescription();
    } else if (rt_code == MCFrameAbstract::WaitingReceive) {
        m_read_buffer.append(receive_bytes);
        return;
    }

    m_retry_count = 0;
    m_wait_for_response = false;
    m_current_request.reset();
}

void McProtocolDevice::retry_request_handle() {
    m_retry_count += 1;
    if (m_retry_count > 5) {
        LOG_USER_ERR << "MC Device Error, disconnect to device";
        deviceDisconnect();
        return;
    }

    if (m_current_request) {
        LOG_USER_ERR << "MC Device Error, request timeout" << m_current_request->type() << m_current_request->m_start_address;
    }
    LOG_USER_INFO << "MC Device Error, timeout, retry" << m_retry_count;
    m_msg_interface->clearBuffer();
    m_wait_for_response = false;
    // m_sent_time_point = std::chrono::high_resolution_clock::now();
    request_handle();
}


void McProtocolDevice::optimizeDeviceMap() {
    m_device_map.clearMap();
    McContext *ctx = m_config.context();
    m_device_map.Subscribe_deivce('M', ctx->m_startMAddress, ctx->m_amountMAddress);
    m_device_map.Subscribe_deivce('D', ctx->m_startDAddress, ctx->m_amountDAddress);
    m_device_map.OptimizeRanges();
    update_m_map();
    update_d_map();
}

void McProtocolDevice::update_m_map() {
    if (m_device_map.m_devices.ranges.empty()) {
        return;
    }

    m_device_map.device_map_m.clear();
    m_last_device_M_map.clear();
    m_m_device_names.clear();

    std::vector<McDeviceRange::DeviceRange> *m_ranges_ptr = &m_device_map.m_devices.ranges;
    int r_size = m_device_map.m_devices.ranges.size();
    for (int index=0;index<r_size;index++) {
        int start_address = (m_ranges_ptr->begin())[index].start;
        int amount = (m_ranges_ptr->begin())[index].amount;
        // init map
        for (int i=0;i<amount;i++) {
            m_device_map.device_map_m[start_address+i] = 0x00;
            m_last_device_M_map[start_address+i] = 0x00;

            QString device_name = QString("M%1").arg(start_address + i, 4, 10, QChar('0'));
            m_m_device_names.append(device_name);
        }

        std::shared_ptr<MCRequest> request = std::make_shared<MCRequest>(
            MCRequest::ReadBit, 'M', start_address, amount);
        m_polling_request_queue.push_back(request);
    }
}

void McProtocolDevice::update_d_map() {
    if (m_device_map.d_devices.ranges.empty()) {
        return;
    }

    m_device_map.device_map_d.clear();
    m_last_device_D_map.clear();
    m_d_device_names.clear();

    std::vector<McDeviceRange::DeviceRange> *d_ranges_ptr = &m_device_map.d_devices.ranges;
    int r_size = m_device_map.d_devices.ranges.size();
    for (int index=0;index<r_size;index++) {
        int start_address = (d_ranges_ptr->begin())[index].start;
        int amount  = (d_ranges_ptr->begin())[index].amount;
        // init map
        for (int i=0;i<amount;i++) {
            m_device_map.device_map_d[start_address+i] = 0x0000;
            m_last_device_D_map[start_address+i] = 0x0000;

            QString device_name = QString("D%1").arg(start_address + i, 4, 10, QChar('0'));
            m_d_device_names.append(device_name);
        }

        std::shared_ptr<MCRequest> request = std::make_shared<MCRequest>(
            MCRequest::ReadWord, 'D', start_address, amount);
        m_polling_request_queue.push_back(request);
    }
}

void McProtocolDevice::check_device_changed() {
    if ((!m_device_map.device_map_m.empty()) && (!m_last_device_M_map.empty())) {

        if (m_device_map.device_map_m.size() != m_last_device_M_map.size()) {
            update_last_m_map();
            LOG_DEV_INFO << "Device M map size change:" << m_last_device_M_map.size();
        }

        std::map<int, quint8>::iterator it_new_m = m_device_map.device_map_m.begin();
        std::map<int, quint8>::iterator it_last_m = m_last_device_M_map.begin();
        for (; it_new_m != m_device_map.device_map_m.end(); ++it_new_m, ++it_last_m) {
            if (it_new_m->second != it_last_m->second) {
                if (!is_first_time_polling) {
                    emit deviceMChanged(it_new_m->first, it_last_m->second, it_new_m->second);
                }
                it_last_m->second = it_new_m->second;
            }
        }
    }

    if ((m_device_map.device_map_d.empty()) || (m_last_device_D_map.empty())) {
        return;
    }
    if (m_device_map.device_map_d.size() != m_last_device_D_map.size()) {
        update_last_d_map();
        LOG_DEV_INFO << "Device D map size change:" << m_last_device_D_map.size();
    }

    std::map<int, qint16>::iterator it_new_d = m_device_map.device_map_d.begin();
    std::map<int, qint16>::iterator it_last_d = m_last_device_D_map.begin();
    for (; it_new_d != m_device_map.device_map_d.end(); ++it_new_d, ++it_last_d) {
        if (it_new_d->second != it_last_d->second) {
            if (!is_first_time_polling) {
                emit deviceDChanged(it_new_d->first, it_last_d->second, it_new_d->second);
            }
            it_last_d->second = it_new_d->second;
        }
    }
}

void McProtocolDevice::update_last_m_map() {
    for (std::map<int, quint8>::iterator it = m_device_map.device_map_m.begin();
         it != m_device_map.device_map_m.end();
         ++it) {

        if (m_last_device_M_map.find(it->first) == m_last_device_M_map.end()){
            m_last_device_M_map[it->first] = it->second;
        }
    }
}

void McProtocolDevice::update_last_d_map() {
    for (std::map<int, qint16>::iterator it = m_device_map.device_map_d.begin();
         it != m_device_map.device_map_d.end();
         ++it) {

        if (m_last_device_D_map.find(it->first) == m_last_device_D_map.end()){
            m_last_device_D_map[it->first] = it->second;
        }
    }
}

}
