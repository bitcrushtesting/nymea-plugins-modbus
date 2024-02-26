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
    Q_UNUSED(info)
    // CionModbusRtuConnection *cionConnection = m_connections.value(info->thing());
    // if (info->action().actionTypeId() == cionPowerActionTypeId) {
    //     bool enabled = info->action().paramValue(cionPowerActionPowerParamTypeId).toBool();
    //     int maxChargingCurrent = enabled ? info->thing()->stateValue(cionMaxChargingCurrentStateTypeId).toUInt() : 0;
    //     qCDebug(dcPcElectric()) << "Setting charging enabled:" << (enabled ? 1 : 0) << "(charging current setpoint:" << maxChargingCurrent << ")";

    //     // Note: If the wallbox has an RFID reader connected, writing register 100 (chargingEnabled) won't work as the RFID
    //     // reader takes control over it. However, if there's no RFID reader connected, we'll have to set it ourselves.
    //     // So summarizing:
    //     // * In setups with RFID reader, we can only control this with register 101 (maxChargingCurrent) by setting it to 0
    //     //   to stop charging, or something >= 6 to allow charging.
    //     // * In setups without RFID reader, we set 100 to true/false. Note that DIP switches 1 & 2 need to be OFF for register
    //     //   100 to be writable.


    //     // In case there's no RFID reader
    //     ModbusRtuReply *reply = cionConnection->setChargingEnabled(enabled);
    //     connect(reply, &ModbusRtuReply::finished, info, [reply](){
    //         qCDebug(dcPcElectric) << "Charging enabled command reply:" << reply->error() << reply->errorString();
    //     });

    //     // And restore the charging current in case setting the above fails
    //     reply = cionConnection->setChargingCurrentSetpoint(maxChargingCurrent);
    //     waitForActionFinish(info, reply, cionPowerStateTypeId, enabled);
    //     connect(reply, &ModbusRtuReply::finished, info, [reply](){
    //         qCDebug(dcPcElectric) << "Implicit max charging current setpoint command reply:" << reply->error() << reply->errorString();
    //     });


    // } else if (info->action().actionTypeId() == cionMaxChargingCurrentActionTypeId) {
    //     // If charging is set to enabled, we'll write the value to the wallbox
    //     uint maxChargingCurrent = info->action().paramValue(cionMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
    //     if (info->thing()->stateValue(cionPowerStateTypeId).toBool()) {
    //         qCDebug(dcPcElectric) << "Charging is enabled. Applying max charging current setpoint of" << maxChargingCurrent << "to wallbox";
    //         ModbusRtuReply *reply = cionConnection->setChargingCurrentSetpoint(maxChargingCurrent);
    //         waitForActionFinish(info, reply, cionMaxChargingCurrentStateTypeId, maxChargingCurrent);
    //         connect(reply, &ModbusRtuReply::finished, info, [reply](){
    //             qCDebug(dcPcElectric) << "Charging current setpoint command reply:" << reply->error() << reply->errorString();
    //         });

    //     } else { // we'll just memorize what the user wants in our state and write it when enabled is set to true
    //         qCDebug(dcPcElectric) << "Charging is disabled, storing max charging current of" << maxChargingCurrent << "to state";
    //         info->thing()->setStateValue(cionMaxChargingCurrentStateTypeId, maxChargingCurrent);
    //         info->finish(Thing::ThingErrorNoError);
    //     }
    // }

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
        qCDebug(dcPcElectric()) << "Reachable changed to" << reachable << "for" << thing;
        thing->setStateValue("connected", reachable);
    });

    connect(connection, &PceWallbox::updateFinished, thing, [thing, connection](){
        qCDebug(dcPcElectric()) << "Update finished for" << thing << connection;
    });

    m_connections.insert(thing, connection);
    info->finish(Thing::ThingErrorNoError);

    // Connect reight the way if the monitor indicates reachable, otherwise the connect will handle the connect later
    if (monitor->reachable())
        connection->connectDevice();
}
