#ifndef MC_MSG_TCP_CLIENT_H
#define MC_MSG_TCP_CLIENT_H

#include "device/plc/mc_msg_interface.h"

#include <memory>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QJsonObject>

namespace vc::device {

class McMsgEthernetTcpCfg : public McMsgItfConfig {
    Q_GADGET

    G_PROPERTY_STRING_READWRITE(QString, ipAddress, "IP Address")
    G_PROPERTY_NUMBER_READWRITE(int, portNumber, 0, 100000, "Port number")

public:
    explicit McMsgEthernetTcpCfg() {

    }

    const QMetaObject &getMetaObject() const override {
        return vc::device::McMsgEthernetTcpCfg::staticMetaObject;
    }

    McMsgItfType type() const override {
        return McMsgItfType::EthernetTCPIP;
    }

    QJsonObject toJson() const override {
        QJsonObject obj = McMsgItfConfig::toJson();
        obj["ipAddress"] = m_ipAddress;
        obj["portNumber"] = m_portNumber;
        return obj;
    }

    bool fromJson(const QJsonObject &obj) override {
        McMsgItfConfig::fromJson(obj);
        m_ipAddress = obj["ipAddress"].toString("192.168.0.1");
        m_portNumber = obj["portNumber"].toInt(5000);
        return true;
    }

public:
    QString m_ipAddress{"192.168.0.1"};
    int m_portNumber{5000};
};

class McEthernetTcpPort : public McMsgInterface {
public:
    McEthernetTcpPort() :
        m_socket(nullptr) {

        m_port_state = MsgIfState::NotInit;
        m_error_state = MsgErrorState::NoError;
        m_error_description = "";
        m_socket = new QTcpSocket;
    }

    ~McEthernetTcpPort() {

    }

    bool SetConfig(McMsgItfConfig *cfg) override {
        if (!cfg) {
            return false;
        }

        if (cfg->type() != McMsgItfType::EthernetTCPIP) {
            return false;
        }

        m_config = *(static_cast<McMsgEthernetTcpCfg*>(cfg));
        return true;
    }

    const bool ConnectionCheck() override {
        // implements connection check later

        return true;
    }

    MsgIfState ConnectToPort() override {
        m_socket->connectToHost(QHostAddress(m_config.m_ipAddress), m_config.m_portNumber);
        m_total_wait_time = 0;

        if (m_socket->waitForConnected(m_config.m_connectTimeout)) {
            m_port_state = MsgIfState::Connected;
        } else {
            m_error_description = "connect to server timeout";
            m_port_state = MsgIfState::ConnectFail;
        }
        return m_port_state;
    }

    MsgIfState DisconnectFromPort() override {
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->disconnectFromHost();
        }
        m_port_state = MsgIfState::NoConnection;
        return m_port_state;
    }

    const MsgErrorState SendMsg(QByteArray &buffer) override {
        if (buffer.isEmpty()) {
            m_error_state = MsgErrorState::BufferEmpty;
            return m_error_state;
        }

        try {
            m_socket->write(buffer);
            if (!m_socket->waitForBytesWritten(m_config.m_writeTimeout)) {
                m_error_state = MsgErrorState::WriteTimeout;
                return m_error_state;
            }
            // OLOG_INFO << "sent string:" << cmd;
        } catch (const std::exception& e){
            // OLOG_CRITICAL << "send string" << cmd << ", fail:" << e.what();
            m_error_state = MsgErrorState::ErrorOcurred;
            m_error_description = QString::fromStdString(e.what());
            return m_error_state;
        }

        m_error_state = MsgErrorState::NoError;
        return m_error_state;
    }

    const MsgErrorState ReceiveMsg(QByteArray &buffer, int wait_buffer = 1) override {        
        m_socket->waitForReadyRead(wait_buffer);

        if (m_socket->bytesAvailable() < 1) {
            m_total_wait_time += wait_buffer;
            if (m_total_wait_time > m_config.m_responseTimeout) {
                m_error_state = MsgErrorState::ResponseTimeout;
                return m_error_state;
            }

            m_error_state = MsgErrorState::BufferEmpty;
            return m_error_state;
        }

        m_total_wait_time = 0;
        QByteArray read_bytes = m_socket->readAll();
        buffer.append(read_bytes);
        m_error_state = MsgErrorState::NoError;
        return m_error_state;
    }

    void clearBuffer() override {
        if(m_socket) {
            m_socket->readAll();
        }
    }

    QString GetErrorDescription() override {
        return m_error_description;
    }

    const McMsgItfType type() const override {
        return McMsgItfType::EthernetTCPIP;
    }

    QIODevice* ioDevice() const override {
        return static_cast<QIODevice*>(m_socket);
    }

    void DestroyMsgPort() override {
        DisconnectFromPort();
        if (m_socket != nullptr) {
            // Delete synchronously on the socket's own (worker) thread. Using
            // deleteLater() here is unsafe during a phase teardown: the worker
            // event loop may be stopped before the deferred delete runs, which
            // leaks the socket and keeps the PLC connection half-open.
            m_socket->abort();
            delete m_socket;
            m_socket = nullptr;
        }
    }

private:
    QTcpSocket *m_socket;
    McMsgEthernetTcpCfg m_config;
    int m_total_wait_time{0};
};

} // namespace vc::device


Q_DECLARE_METATYPE(vc::device::McMsgEthernetTcpCfg)

#endif // MC_MSG_TCP_CLIENT_H
