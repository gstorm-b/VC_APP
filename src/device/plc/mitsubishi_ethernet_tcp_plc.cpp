// #include "mitsubishi_ethernet_tcp_plc.h"]
// #include "device/plc/mc_fame_3e.h"

// namespace vc::device {


// QJsonObject MCCfg::toJson() const {
//     QJsonObject obj;
//     obj["DataCode"] = static_cast<int>(dataCode());
//     obj["FrameType"] = static_cast<int>(frameType());
//     obj["RefreshInterval"] = refresh_interval;
//     obj["StartMAddress"] = start_m_address;
//     obj["AmountMAddress"] = amount_m_devices;
//     obj["StartDAddress"] = start_d_address;
//     obj["AmountDAddress"] = amount_d_devices;
//     return obj;
// }

// bool MCCfg::fromJson(const QJsonObject &obj) {
//     data_code        = static_cast<McDataCode>(obj["DataCode"].toInt());
//     // frame_type       = static_cast<McFrameE>(obj["FrameType"].toInt());
//     refresh_interval = obj["RefreshInterval"].toInt();
//     start_m_address  = obj["StartMAddress"].toInt();
//     amount_m_devices = obj["AmountMAddress"].toInt();
//     start_d_address  = obj["StartDAddress"].toInt();
//     amount_d_devices = obj["AmountDAddress"].toInt();
//     return true;
// }

// QJsonObject MC3ECfg::toJson() const {
//     QJsonObject obj = MCCfg::toJson();
//     obj["IpAddress"] = ip_address;
//     obj["PortNumber"] = port_number;
//     obj["FrameContext"] = frame_context.toJson();
//     return obj;
// }

// bool MC3ECfg::fromJson(const QJsonObject &obj) {
//     MCCfg::fromJson(obj);
//     ip_address       = obj["IpAddress"].toString();
//     port_number      = obj["PortNumber"].toInt();
//     frame_context.fromJson(obj["FrameContext"].toObject());
//     return true;
// }

// MCEthernetTCPPlc::MCEthernetTCPPlc(QString id, QString name, QObject* parent)
//     : vc::device::PLCDevice(id, name, parent) {

//     m_polling_timer = new QTimer(this);
//     m_polling_timer->setTimerType(Qt::PreciseTimer);

//     connect(m_polling_timer, &QTimer::timeout,
//             this, &MCEthernetTCPPlc::onPollingTimeOut);
// }


// bool MCEthernetTCPPlc::plcInitialize() {
//     QMutexLocker locker(&m_mutex);

//     return false;
// }

// void MCEthernetTCPPlc::plcTerminate() {

// }

// bool MCEthernetTCPPlc::deviceConnect() {
//     QMutexLocker locker(&m_mutex);

//     if (m_msg_interface) {
//         m_msg_interface.release();
//     }
//     m_msg_interface.reset(std::move(new PlcEthernetTcpPort()));
//     LOG_DEV_INFO << "ethernet tcp/ip interface created";
//     if (m_msg_interface == nullptr) {
//         LOG_DEV_ERR << "PLC Communicator: message interface is null.";
//         return false;
//     }

//     switch (m_config->frameType()) {
//     case vc::device::McFrame::Frame_1C:
//         LOG_DEV_ERR << "Not supported Frame 1C yet!";
//         return false;
//     case vc::device::McFrame::Frame_3C:
//         LOG_DEV_ERR << "Not supported Frame 3C yet!";
//         return false;
//     case vc::device::McFrame::Frame_1E:
//         LOG_DEV_ERR << "Not supported Frame 1E yet!";
//         return false;
//     case vc::device::McFrame::Frame_3E:
//         m_context = std::make_shared<Context_Mc3E>();
//         std::dynamic_pointer_cast<Context_Mc3E>(m_context)->setDeviceMap(&m_device_map);
//         QJsonObject context_obj = m_config->toJson();
//         m_context->fromJson(context_obj["FrameContext"].toObject());
//         m_frame = std::make_unique<Frame3E>();


//         MC3ECfg *cfg = static_cast<MC3ECfg*>(m_config.get());
//         m_msg_interface->setParams(cfg->ip_address, cfg->port_number);

//         m_device_map.Subscribe_deivce('M', cfg->start_m_address, cfg->amount_m_devices);
//         m_device_map.Subscribe_deivce('D',cfg->start_d_address, cfg->amount_d_devices);
//         m_device_map.OptimizeRanges();

//         LOG_DEV_ERR << "PLC Communicator selected 3E frame.";
//         LOG_DEV_ERR << "Connect to " << cfg->ip_address << cfg->port_number;
//         break;
//     }

//     m_msg_interface->SetConnectTimeout(5000);
//     PlcMsgInterface::MsgIfState msg_port_state = m_msg_interface->ConnectToPort();
//     if (msg_port_state != PlcMsgInterface::MsgIfState::Connected) {
//         m_last_msg = tr("PLC communicator connect error occurred, %1").arg(m_msg_interface->GetErrorDescription());
//         LOG_USER_ERR << m_last_msg;
//         return false;
//     }
//     m_last_msg = tr("PLC communicator connected");
//     LOG_USER_ERR << m_last_msg;

//     QByteArray send_frame;
//     std::shared_ptr<PlcRequest> request = std::make_shared<MCRequest>(
//         MCRequest::ReadWord, 'D', 500, 16);
//     m_current_request = request;
//     MCFrameAbstract::FrameReturnCode rt_code = m_frame->makeSendFrame(request.get(), m_context.get(), send_frame);

//     if (rt_code == MCFrameAbstract::RequestFrameOK) {
//         PlcMsgInterface::MsgErrorState send_state = m_msg_interface->SendMsg(send_frame);

//         if (send_state != PlcMsgInterface::MsgErrorState::NoErorr) {
//             LOG_DEV_INFO << "error while send frame";
//             return false;
//         }

//         // m_wait_for_response = true;
//         LOG_DEV_INFO << "Send frame to PLC:" << send_frame.toHex(' ');
//     } else {
//         // m_current_request.reset();
//         LOG_DEV_INFO << "make request frame error";
//     }

//     connect(m_msg_interface->ioDevice(), &QIODevice::readyRead,
//             this, &MCEthernetTCPPlc::socketReceived, Qt::SingleShotConnection);

//     return false;
// }

// bool MCEthernetTCPPlc::deviceDisconnect() {
//     if (m_msg_interface != 0) {
//         m_msg_interface->DestroyMsgPort();
//     }

//     m_msg_interface.release();
//     m_context.reset();
//     m_frame.release();
//     return true;
// }


// QJsonObject MCEthernetTCPPlc::toJson() const {
//     QJsonObject obj = PLCDevice::toJson();
//     obj[DEVICE_JSK_CONFIG] = m_config->toJson();
//     return obj;
// }

// void MCEthernetTCPPlc::setConfigJson(const QJsonObject& obj) {
//     McFrame frame_type = static_cast<McFrame>(obj["FrameType"].toInt());

//     if (m_config ? (m_config->frameType() == frame_type) : false) {
//         m_config->fromJson(obj);
//     } else {
//         if (frame_type == McFrame::Frame_3E) {
//             MC3ECfg *cfg = new MC3ECfg();
//             cfg->fromJson(obj);
//             m_config.reset(cfg);
//         } else {
//             LOG_DEV_ERR << "Wrong PLC Json config";
//             return;
//         }
//     }
//     IDevice::setConfigJson(obj);
// }

// bool MCEthernetTCPPlc::pushRequest(PlcRequest *request) {
//     QMutexLocker locker(&m_mutex);
//     m_request_queue.push_back(request->clone());
//     static_cast<MCRequest*>(m_request_queue.last().get())->m_value.detach();
//     return true;
// }

// std::shared_ptr<MCCfg> MCEthernetTCPPlc::McEthernetConfig() {
//     QMutexLocker locker(&m_mutex);
//     if (!m_config) {
//         return nullptr;
//     }

//     return m_config->clone();
// }

// void MCEthernetTCPPlc::setMcEthernetConfig(MCCfg* cfg) {
//     QMutexLocker locker(&m_mutex);

//     if (m_config ? (m_config->frameType() == cfg->frameType()) : false) {
//         m_config->fromJson(cfg->toJson());
//     } else {
//         if (cfg->frameType() == McFrame::Frame_3E) {
//             MC3ECfg *new_cfg = new MC3ECfg();
//             new_cfg->fromJson(cfg->toJson());
//             m_config.reset(new_cfg);
//         }
//     }
// }

// void MCEthernetTCPPlc::onPollingTimeOut() {
//     // start polling query
// }

// void MCEthernetTCPPlc::socketReceived() {
//     if (m_current_request == nullptr) {
//         return;
//     }

//     QByteArray receive_bytes;
//     PlcMsgInterface::MsgErrorState recieve_state = m_msg_interface->ReceiveMsg(receive_bytes, 5);
//     // OLOG_INFO << "Received frame from PLC:" << receive_bytes.toHex(' ');

//     if (recieve_state != PlcMsgInterface::MsgErrorState::NoErorr) {
//         if (recieve_state == PlcMsgInterface::MsgErrorState::BufferEmpty) {
//             LOG_DEV_INFO << "empty frame";
//             return;
//         } else if (recieve_state == PlcMsgInterface::MsgErrorState::ResponseTimeout) {
//             LOG_DEV_INFO << "timeout";
//             return;
//         }
//         LOG_DEV_INFO << "error while receive frame";
//         return;
//     }

//     MCFrameAbstract::FrameReturnCode rt_code = m_frame->parseReceiveFrame(m_current_request.get(), m_context.get(), receive_bytes);

//     if (rt_code == MCFrameAbstract::ResponseOk) {
//         // OLOG_INFO << "reponse frame status error," << m_frame->lastErrorDescription();
//         check_device_changed();

//     } else if  (rt_code == MCFrameAbstract::ResponseError) {
//         LOG_DEV_INFO << "reponse frame status error," << m_frame->lastErrorDescription();
//     } else if  (rt_code == MCFrameAbstract::ResponseInvalid) {
//         LOG_DEV_INFO << "reponse frame invalid," << m_frame->lastErrorDescription();
//     } else if (rt_code == MCFrameAbstract::WaitingReceive) {
//         // m_read_buffer.append(receive_bytes);
//         return;
//     }

//     deviceDisconnect();
// }

// void MCEthernetTCPPlc::check_device_changed() {
//     if ((!m_device_map.device_map_m.empty()) && (!m_last_device_M_map.empty())) {

//         if (m_device_map.device_map_m.size() != m_last_device_M_map.size()) {
//             update_last_m_map();
//             LOG_DEV_INFO << "Device M map size change:" << m_last_device_M_map.size();
//         }

//         std::map<int, quint8>::iterator it_new_m = m_device_map.device_map_m.begin();
//         std::map<int, quint8>::iterator it_last_m = m_last_device_M_map.begin();
//         for (; it_new_m != m_device_map.device_map_m.end(); ++it_new_m, ++it_last_m) {
//             if (it_new_m->second != it_last_m->second) {
//                 // if (!is_first_time_polling) {
//                 //     emit s_Device_M_Changed(it_new_m->first, it_last_m->second, it_new_m->second);
//                 // }
//                 it_last_m->second = it_new_m->second;
//             }
//         }
//     }

//     if ((m_device_map.device_map_d.empty()) || (m_last_device_D_map.empty())) {
//         return;
//     }
//     if (m_device_map.device_map_d.size() != m_last_device_D_map.size()) {
//         update_last_d_map();
//         LOG_DEV_INFO << "Device D map size change:" << m_last_device_D_map.size();
//     }

//     std::map<int, qint16>::iterator it_new_d = m_device_map.device_map_d.begin();
//     std::map<int, qint16>::iterator it_last_d = m_last_device_D_map.begin();
//     for (; it_new_d != m_device_map.device_map_d.end(); ++it_new_d, ++it_last_d) {
//         if (it_new_d->second != it_last_d->second) {
//             // if (!is_first_time_polling) {
//             //     emit s_Device_D_Changed(it_new_d->first, it_last_d->second, it_new_d->second);
//             // }
//             it_last_d->second = it_new_d->second;
//         }
//     }
// }

// void MCEthernetTCPPlc::update_last_m_map() {
//     for (std::map<int, quint8>::iterator it = m_device_map.device_map_m.begin();
//          it != m_device_map.device_map_m.end();
//          ++it) {

//         if (m_last_device_M_map.find(it->first) == m_last_device_M_map.end()){
//             m_last_device_M_map[it->first] = it->second;
//         }
//     }
// }

// void MCEthernetTCPPlc::update_last_d_map() {
//     for (std::map<int, qint16>::iterator it = m_device_map.device_map_d.begin();
//          it != m_device_map.device_map_d.end();
//          ++it) {

//         if (m_last_device_D_map.find(it->first) == m_last_device_D_map.end()){
//             m_last_device_D_map[it->first] = it->second;
//         }
//     }
// }

// void MCEthernetTCPPlc::timerStart() {
//     m_polling_timer->setInterval(m_config->refresh_interval);
//     m_polling_timer->start();
// }

// void MCEthernetTCPPlc::timerStop() {
//     m_polling_timer->stop();
// }


// }
