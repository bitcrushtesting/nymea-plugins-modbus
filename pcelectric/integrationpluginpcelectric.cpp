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

#include "integrationpluginpcelectric.h"
#include "pcelectricdiscovery.h"
#include "plugininfo.h"

#include <hardwaremanager.h>
#include <hardware/electricity.h>

IntegrationPluginPcElectric::IntegrationPluginPcElectric()
{

}

void IntegrationPluginPcElectric::init()
{

}

void IntegrationPluginPcElectric::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcPcElectric()) << "The network discovery is not available on this platform.";
        info->finish(Thing::ThingErrorUnsupportedFeature, QT_TR_NOOP("The network device discovery is not available."));
        return;
    }

    // Create a discovery with the info as parent for auto deleting the object once the discovery info is done
    PcElectricDiscovery *discovery = new PcElectricDiscovery(hardwareManager()->networkDeviceDiscovery(), 502, 1, info);
    connect(discovery, &PcElectricDiscovery::discoveryFinished, info, [=](){
        foreach (const PcElectricDiscovery::Result &result, discovery->results()) {

            ThingDescriptor descriptor(ev11ThingClassId, "PCE EV11.3 (" + result.serialNumber + ")", "Version: " + result.firmwareRevision + " - " + result.networkDeviceInfo.address().toString());
            qCDebug(dcPcElectric()) << "Discovered:" << descriptor.title() << descriptor.description();

            // Check if we already have set up this device
            Things existingThings = myThings().filterByParam(ev11ThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
                qCDebug(dcPcElectric()) << "This PCE wallbox already exists in the system:" << result.networkDeviceInfo;
                descriptor.setThingId(existingThings.first()->id());
            }

            ParamList params;
            params << Param(ev11ThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
            // Note: if we discover also the port and modbusaddress, we must fill them in from the discovery here, for now everywhere the defaults...
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }

        info->finish(Thing::ThingErrorNoError);
    });

    // Start the discovery process
    discovery->startDiscovery();
}

void IntegrationPluginPcElectric::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcPcElectric()) << "Setup thing" << thing << thing->params();

    if (m_connections.contains(thing)) {
        qCDebug(dcPcElectric()) << "Reconfiguring existing thing" << thing->name();
        m_connections.take(thing)->deleteLater();

        if (m_monitors.contains(thing)) {
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        }
    }

    MacAddress macAddress = MacAddress(thing->paramValue(ev11ThingMacAddressParamTypeId).toString());
    if (!macAddress.isValid()) {
        qCWarning(dcPcElectric()) << "The configured mac address is not valid" << thing->params();
        info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The MAC address is not known. Please reconfigure the thing."));
        return;
    }

    NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(macAddress);
    m_monitors.insert(thing, monitor);

    connect(info, &ThingSetupInfo::aborted, monitor, [=](){
        if (m_monitors.contains(thing)) {
            qCDebug(dcPcElectric()) << "Unregistering monitor because setup has been aborted.";
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
        }
    });

    // Only make sure the connection is working in the initial setup, otherwise we let the monitor do the work
    if (info->isInitialSetup()) {
        // Continue with setup only if we know that the network device is reachable
        if (monitor->reachable()) {
            setupConnection(info);
        } else {
            // otherwise wait until we reach the networkdevice before setting up the device
            qCDebug(dcPcElectric()) << "Network device" << thing->name() << "is not reachable yet. Continue with the setup once reachable.";
            connect(monitor, &NetworkDeviceMonitor::reachableChanged, info, [=](bool reachable){
                if (reachable) {
                    qCDebug(dcPcElectric()) << "Network device" << thing->name() << "is now reachable. Continue with the setup...";
                    setupConnection(info);
                }
            });
        }
    } else {
        setupConnection(info);
    }

    return;
}

void IntegrationPluginPcElectric::postSetupThing(Thing *thing)
{
    qCDebug(dcPcElectric()) << "Post setup thing" << thing->name();
    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_refreshTimer, &PluginTimer::timeout, this, [this] {
            foreach (PceWallbox *connection, m_connections) {
                if (connection->reachable()) {
                    connection->update();
                }
            }
        });

        qCDebug(dcPcElectric()) << "Starting refresh timer...";
        m_refreshTimer->start();
    }
}

void IntegrationPluginPcElectric::thingRemoved(Thing *thing)
{
    qCDebug(dcPcElectric()) << "Thing removed" << thing->name();

    if (m_connections.contains(thing)) {
        PceWallbox *connection = m_connections.take(thing);
        connection->disconnectDevice();
        connection->deleteLater();
    }

    // Unregister related hardware resources
    if (m_monitors.contains(thing))
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));

    if (myThings().isEmpty() && m_refreshTimer) {
        qCDebug(dcPcElectric()) << "Stopping reconnect timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginPcElectric::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();

    PceWallbox *connection = m_connections.value(thing);
    if (!connection->reachable()) {
        qCWarning(dcPcElectric()) << "Could not execute action because the connection is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (info->action().actionTypeId() == ev11PowerActionTypeId) {
        bool power = info->action().paramValue(ev11PowerActionPowerParamTypeId).toBool();
        quint16 chargingCurrent = 0;
        if (power) {
            chargingCurrent = thing->stateValue(ev11MaxChargingCurrentStateTypeId).toUInt() * 1000;
            if (thing->stateValue(ev11DesiredPhaseCountStateTypeId).toUInt() == 3) {
                // If 3 phase charging is enabled, we set the first bit
                chargingCurrent |= static_cast<quint16>(1) << 15;
            }
        }

        qCDebug(dcPcElectric()) << "Writing charging current register" << chargingCurrent << "mA";
        QueuedModbusReply *reply = connection->setChargingCurrent(chargingCurrent);
        connect(reply, &QueuedModbusReply::finished, info, [reply, info, thing, power, chargingCurrent](){
            if (reply->error() != QModbusDevice::NoError) {
                qCWarning(dcPcElectric()) << "Could not set power state to" << power << "(" << chargingCurrent << "mA)" << reply->errorString();
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }

            qCDebug(dcPcElectric()) << "Successfully set power state to" << power << "(" << chargingCurrent << "mA)";
            thing->setStateValue(ev11PowerStateTypeId, power);
            info->finish(Thing::ThingErrorNoError);
        });
        return;
    } else if (info->action().actionTypeId() == ev11MaxChargingCurrentActionTypeId) {
        uint desiredChargingCurrent = info->action().paramValue(ev11MaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
        qCDebug(dcPcElectric()) << "Set max charging current to" << desiredChargingCurrent << "A";
        if (thing->stateValue(ev11PowerStateTypeId).toBool()) {
            // The charging is enabled, let's write the value to the wallbox
            quint16 finalChargingCurrent = static_cast<quint16>(desiredChargingCurrent * 1000);
            if (thing->stateValue(ev11DesiredPhaseCountStateTypeId).toUInt() == 3) {
                // If 3 phase charging is enabled, we set the first bit
                finalChargingCurrent |= static_cast<quint16>(1) << 15;
            }

            qCDebug(dcPcElectric()) << "Writing charging current register" << finalChargingCurrent << "mA";
            QueuedModbusReply *reply = connection->setChargingCurrent(finalChargingCurrent);
            connect(reply, &QueuedModbusReply::finished, info, [reply, info, thing, desiredChargingCurrent](){
                if (reply->error() != QModbusDevice::NoError) {
                    qCWarning(dcPcElectric()) << "Could not set charging current to" << desiredChargingCurrent << "mA" << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                    return;
                }

                qCDebug(dcPcElectric()) << "Successfully set charging current to" << desiredChargingCurrent << "mA";
                thing->setStateValue(ev11MaxChargingCurrentStateTypeId, desiredChargingCurrent);
                info->finish(Thing::ThingErrorNoError);
            });
        } else {
            // Save the value in the state, but do not send the value to the wallbox since the power state is reflected using the charging current...
            qCDebug(dcPcElectric()) << "Setting charging current to" << desiredChargingCurrent << "without synching to wallbox since the power state is false";
            thing->setStateValue(ev11MaxChargingCurrentStateTypeId, desiredChargingCurrent);
            info->finish(Thing::ThingErrorNoError);
        }
        return;
    } else if (info->action().actionTypeId() == ev11DesiredPhaseCountActionTypeId) {
        thing->setStateValue(ev11DesiredPhaseCountStateTypeId, info->action().paramValue(ev11DesiredPhaseCountActionDesiredPhaseCountParamTypeId).toUInt());
        info->finish(Thing::ThingErrorNoError);
        return;
    }


    Q_ASSERT_X(false, "IntegrationPluginPcElectric::executeAction", QString("Unhandled action: %1").arg(info->action().actionTypeId().toString()).toLocal8Bit());
}

void IntegrationPluginPcElectric::setupConnection(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    NetworkDeviceMonitor *monitor = m_monitors.value(thing);

    qCDebug(dcPcElectric()) << "Setting up PCE wallbox finished successfully" << monitor->networkDeviceInfo().address().toString();

    PceWallbox *connection = new PceWallbox(monitor->networkDeviceInfo().address(), 502, 1, this);
    connect(info, &ThingSetupInfo::aborted, connection, &PceWallbox::deleteLater);

    // Monitor reachability
    connect(monitor, &NetworkDeviceMonitor::reachableChanged, thing, [=](bool reachable){
        if (!thing->setupComplete())
            return;

        qCDebug(dcPcElectric()) << "Network device monitor for" << thing->name() << (reachable ? "is now reachable" : "is not reachable any more" );
        if (reachable && !thing->stateValue("connected").toBool()) {
            connection->modbusTcpMaster()->setHostAddress(monitor->networkDeviceInfo().address());
            connection->connectDevice();
        } else if (!reachable) {
            // Note: We disable autoreconnect explicitly and we will
            // connect the device once the monitor says it is reachable again
            connection->disconnectDevice();
        }
    });

    // Connection reachability
    connect(connection, &PceWallbox::reachableChanged, thing, [thing](bool reachable){
        qCInfo(dcPcElectric()) << "Reachable changed to" << reachable << "for" << thing;
        thing->setStateValue("connected", reachable);
    });

    connect(connection, &PceWallbox::updateFinished, thing, [thing, connection](){
        qCDebug(dcPcElectric()) << "Update finished for" << thing;
        qCDebug(dcPcElectric()) << connection;
        if (!connection->phaseAutoSwitch()) {
            // Note: if auto phase switching is disabled, the wallbox forces 3 phase charging
            thing->setStatePossibleValues(ev11DesiredPhaseCountStateTypeId, { 3 }); // Disable switching to one phase
            thing->setStateValue(ev11DesiredPhaseCountStateTypeId, 3);
            thing->setStateValue(ev11PhaseCountStateTypeId, 3);
        } else {
            thing->setStatePossibleValues(ev11DesiredPhaseCountStateTypeId, { 1, 3 }); // Phase switching
        }

        if (connection->chargingRelayState() != EV11ModbusTcpConnection::ChargingRelayStateNoCharging) {
            if (connection->chargingRelayState() == EV11ModbusTcpConnection::ChargingRelayStateSinglePhase) {
                thing->setStateValue(ev11PhaseCountStateTypeId, 1);
            } else if (connection->chargingRelayState() == EV11ModbusTcpConnection::ChargingRelayStateTheePhase) {
                thing->setStateValue(ev11PhaseCountStateTypeId, 3);
            }
        }

        thing->setStateMaxValue(ev11MaxChargingCurrentStateTypeId, connection->maxChargingCurrentDip() / 1000);
        thing->setStateValue(ev11PluggedInStateTypeId, connection->chargingState() >= PceWallbox::ChargingStateB1 &&
                                                           connection->chargingState() < PceWallbox::ChargingStateError);


        thing->setStateValue(ev11ChargingStateTypeId, connection->chargingState() == PceWallbox::ChargingStateC2);
        thing->setStateValue(ev11CurrentVersionStateTypeId, connection->firmwareRevision());
        thing->setStateValue(ev11SessionEnergyStateTypeId, connection->powerMeter0());
        thing->setStateValue(ev11TemperatureStateTypeId, connection->temperature());

        // ErrorOverheating = 1,
        //     ErrorDCFaultCurrent = 2,
        //     ErrorChargingWithVentilation = 3,
        //     ErrorCPErrorEF = 4,
        //     ErrorCPErrorBypass = 5,
        //     ErrorCPErrorDiodFault = 6,
        //     ErrorDCFaultCurrentCalibrating = 7,
        //     ErrorDCFaultCurrentCommunication = 8,
        //     ErrorDCFaultCurrentError = 9

        switch (connection->error()) {
        case EV11ModbusTcpConnection::ErrorNoError:
            thing->setStateValue(ev11ErrorStateTypeId, "Kein Fehler aktiv");
            break;
        case EV11ModbusTcpConnection::ErrorOverheating:
            thing->setStateValue(ev11ErrorStateTypeId, "Übertemperatur. Ladevorgang wird automatisch fortgesetzt.");
            break;
        case EV11ModbusTcpConnection::ErrorDCFaultCurrent:
            thing->setStateValue(ev11ErrorStateTypeId, "DC Fehlerstromsensor ausgelöst.");
            break;
        case EV11ModbusTcpConnection::ErrorChargingWithVentilation:
            thing->setStateValue(ev11ErrorStateTypeId, "Ladeanforderung mit Belüftung.");
            break;
        case EV11ModbusTcpConnection::ErrorCPErrorEF:
            thing->setStateValue(ev11ErrorStateTypeId, "CP Signal, Fehlercode E oder F.");
            break;
        case EV11ModbusTcpConnection::ErrorCPErrorBypass:
            thing->setStateValue(ev11ErrorStateTypeId, "CP Signal, bypass.");
            break;
        case EV11ModbusTcpConnection::ErrorCPErrorDiodFault:
            thing->setStateValue(ev11ErrorStateTypeId, "CP Signal, Diode defekt.");
            break;
        case EV11ModbusTcpConnection::ErrorDCFaultCurrentCalibrating:
            thing->setStateValue(ev11ErrorStateTypeId, "DC Fehlerstromsensor, Kalibrirung.");
            break;
        case EV11ModbusTcpConnection::ErrorDCFaultCurrentCommunication:
            thing->setStateValue(ev11ErrorStateTypeId, "DC Fehlerstromsensor, Kommunikationsfehler.");
            break;
        case EV11ModbusTcpConnection::ErrorDCFaultCurrentError:
            thing->setStateValue(ev11ErrorStateTypeId, "DC Fehlerstromsensor, Fehler.");
            break;
        }
    });

    m_connections.insert(thing, connection);
    info->finish(Thing::ThingErrorNoError);

    // Connect reight the way if the monitor indicates reachable, otherwise the connect will handle the connect later
    if (monitor->reachable())
        connection->connectDevice();
}
