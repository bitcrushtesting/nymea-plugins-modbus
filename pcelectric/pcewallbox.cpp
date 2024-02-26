/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "pcewallbox.h"
#include "extern-plugininfo.h"

PceWallbox::PceWallbox(const QHostAddress &hostAddress, uint port, quint16 slaveId, QObject *parent)
    : EV11ModbusTcpConnection{hostAddress, port, slaveId, parent}
{
    // Timer for resetting the heartbeat register (watchdog)
    m_timer.setInterval(30000);
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, [this](){
        m_resetWatchdog = true;
    });

    connect(this, &EV11ModbusTcpConnection::reachableChanged, this, [this](bool reachable){
        if (!reachable) {
            m_timer.stop();
            m_resetWatchdog = false;
            disconnectDevice();

            QTimer::singleShot(2000, this, &EV11ModbusTcpConnection::connectDevice);
        } else {
            initialize();
        }
    });

    connect(this, &EV11ModbusTcpConnection::initializationFinished, this, [this](bool success){
        if (success) {
            qCDebug(dcPcElectric()) << "Connection initialized successfully" << m_modbusTcpMaster->hostAddress().toString();
            m_timer.start();
            m_resetWatchdog = true;
            update();
        } else {
            qCWarning(dcPcElectric()) << "Connection initialization failed for" << m_modbusTcpMaster->hostAddress().toString();
        }
    });
}

bool PceWallbox::update()
{
    if (!reachable())
        return false;

    if (m_currentReply)
        return false;

    // No need to reset the watchdog...let's just update
    if (!m_resetWatchdog)
        return EV11ModbusTcpConnection::update();

    // First reset the watchdog, then update...
    m_currentReply = setHeartbeat(1);
    connect(m_currentReply, &QModbusReply::finished, this, [this](){
        QModbusResponse response = m_currentReply->rawResult();

        if (m_currentReply->error() == QModbusDevice::NoError) {
            qCDebug(dcPcElectric()) << "Write \"Heartbeat (write < 60s to keep alive)\" finished successfully.";
        } else {
            if (m_currentReply->error() == QModbusDevice::ProtocolError && response.isException()) {
                qCWarning(dcPcElectric()) << "Modbus reply error occurred while writing \"Heartbeat (write < 60s to keep alive)\" register" << m_currentReply->errorString() << ModbusDataUtils::exceptionCodeToString(response.exceptionCode());
            } else {
                qCWarning(dcPcElectric()) << "Modbus reply error occurred while writing \"Heartbeat (write < 60s to keep alive)\" register" << m_currentReply->errorString();
            }
        }

        m_resetWatchdog = false;

        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        EV11ModbusTcpConnection::update();
    });

    return true;
}
