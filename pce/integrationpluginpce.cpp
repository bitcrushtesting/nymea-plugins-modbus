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

#include "integrationpluginpce.h"
#include "plugininfo.h"

#include <hardware/electricity.h>

IntegrationPluginPce::IntegrationPluginPce()
{
}

void IntegrationPluginPce::init()
{

}

void IntegrationPluginPce::discoverThings(ThingDiscoveryInfo *info)
{
    Q_UNUSED(info)
    // CionDiscovery *discovery = new CionDiscovery(hardwareManager()->modbusRtuResource(), info);

    // connect(discovery, &CionDiscovery::discoveryFinished, info, [this, info, discovery](bool modbusMasterAvailable){
    //     if (!modbusMasterAvailable) {
    //         info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("No modbus RTU master with appropriate settings found. Please set up a modbus RTU master with a baudrate of 57600, 8 data bis, 1 stop bit and no parity first."));
    //         return;
    //     }

    //     qCInfo(dcPce()) << "Discovery results:" << discovery->discoveryResults().count();

    //     foreach (const CionDiscovery::Result &result, discovery->discoveryResults()) {
    //         ThingDescriptor descriptor(cionThingClassId, "Schrack CION", QString("Slave ID: %1, Version: %2").arg(result.slaveId).arg(result.firmwareVersion));

    //         ParamList params{
    //             {cionThingModbusMasterUuidParamTypeId, result.modbusRtuMasterId},
    //             {cionThingSlaveAddressParamTypeId, result.slaveId}
    //         };
    //         descriptor.setParams(params);

    //         Thing *existingThing = myThings().findByParams(params);
    //         if (existingThing) {
    //             descriptor.setThingId(existingThing->id());
    //         }
    //         info->addThingDescriptor(descriptor);
    //     }

    //     info->finish(Thing::ThingErrorNoError);
    // });

    // discovery->startDiscovery();
}

void IntegrationPluginPce::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcPce()) << "Setup thing" << thing << thing->params();

//     uint address = thing->paramValue(cionThingSlaveAddressParamTypeId).toUInt();
//     if (address > 254 || address == 0) {
//         qCWarning(dcPce()) << "Setup failed, slave address is not valid" << address;
//         info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("The Modbus address not valid. It must be a value between 1 and 254."));
//         return;
//     }

//     QUuid uuid = thing->paramValue(cionThingModbusMasterUuidParamTypeId).toUuid();
//     if (!hardwareManager()->modbusRtuResource()->hasModbusRtuMaster(uuid)) {
//         qCWarning(dcPce()) << "Setup failed, hardware manager not available";
//         info->finish(Thing::ThingErrorSetupFailed, QT_TR_NOOP("The Modbus RTU resource is not available."));
//         return;
//     }

//     if (m_connections.contains(thing)) {
//         qCDebug(dcPce()) << "Already have a CION connection for this thing. Cleaning up old connection and initializing new one...";
//         delete m_connections.take(thing);
//     }

//     CionModbusRtuConnection *cionConnection = new CionModbusRtuConnection(hardwareManager()->modbusRtuResource()->getModbusRtuMaster(uuid), address, this);
//     if (!info->isInitialSetup()) {
//         m_connections.insert(thing, cionConnection);
//         info->finish(Thing::ThingErrorNoError);
//     }

//     connect(cionConnection, &CionModbusRtuConnection::reachableChanged, thing, [cionConnection, thing](bool reachable){
//         qCDebug(dcPce()) << "Reachable state changed" << reachable;
//         if (reachable) {
//             cionConnection->initialize();
//         } else {
//             thing->setStateValue("connected", false);
//         }
//     });

//     connect(cionConnection, &CionModbusRtuConnection::initializationFinished, info, [=](bool success){
//         qCDebug(dcPce()) << "Initialisation finished" << success << "DIP switche states:" << cionConnection->dipSwitches();
//         if (info->isInitialSetup()) {
//             if (success) {
//                 m_connections.insert(thing, cionConnection);
//                 info->finish(Thing::ThingErrorNoError);
//                 info->thing()->setStateValue(cionCurrentVersionStateTypeId, cionConnection->firmwareVersion());
//             } else {
//                 delete cionConnection;
//                 info->finish(Thing::ThingErrorHardwareNotAvailable);
//             }
//         }
//     });

//     connect(cionConnection, &CionModbusRtuConnection::updateFinished, thing, [cionConnection, thing](){
//         qCDebug(dcPce()) << "Update finished:" << thing->name() << cionConnection;
//     });

//     connect(cionConnection, &CionModbusRtuConnection::reachableChanged, thing, [thing](bool reachable){
//         qCDebug(dcPce()) << "Reachable changed:" << thing->name() << reachable;
//     });


//     // Note: This register really only tells us if we can control anything... i.e. if the wallbox is unlocked via RFID
//     connect(cionConnection, &CionModbusRtuConnection::chargingEnabledChanged, thing, [=](quint16 charging){
//         qCDebug(dcPce()) << "Charge control enabled changed:" << charging;
//         // If this register goes 0->1 it means charging has been started. This could be because of an RFID tag.
//         // As we have may set charging current to 0 ourselves, we'll want to activate it again here
//         uint maxSetPoint = thing->stateValue(cionMaxChargingCurrentStateTypeId).toUInt();
//         if (cionConnection->chargingCurrentSetpoint() != maxSetPoint) {
//             cionConnection->setChargingCurrentSetpoint(maxSetPoint);
//         }
//     });

//     // We can write chargingCurrentSetpoint to the preferred charging we want, and the wallbox will take it,
//     // however, it may not necessarily *do* it, but will give us the actual value it uses in currentChargingCurrentE3
//     // We'll use that for setting our state, just monitoring this one on the logs
//     // Setting this to 0 will pause charging, anything else will control the charging (and return the actual value in currentChargingCurrentE3)
//     connect(cionConnection, &CionModbusRtuConnection::chargingCurrentSetpointChanged, thing, [=](quint16 chargingCurrentSetpoint){
//         qCDebug(dcPce()) << "Charging current setpoint changed:" << chargingCurrentSetpoint;
//     });

//     connect(cionConnection, &CionModbusRtuConnection::cpSignalStateChanged, thing, [=](quint16 cpSignalState){
//         qCDebug(dcPce()) << "CP Signal state changed:" << (char)cpSignalState;
//         if (cpSignalState < 65 || cpSignalState > 68) {
//             qCWarning(dcPce()) << "Ignoring bogus CP signal state value" << cpSignalState;
//             return;
//         }
//         thing->setStateValue(cionPluggedInStateTypeId, cpSignalState >= 66);
//     });

//     connect(cionConnection, &CionModbusRtuConnection::currentChargingCurrentE3Changed, thing, [=](quint16 currentChargingCurrentE3){
//         qCDebug(dcPce()) << "Current charging current E3 current changed:" << currentChargingCurrentE3;
//         if (cionConnection->chargingEnabled() == 1 && cionConnection->chargingCurrentSetpoint() > 0) {
//             thing->setStateValue(cionMaxChargingCurrentStateTypeId, currentChargingCurrentE3);
//             thing->setStateValue(cionPowerStateTypeId, true);
//         } else {
//             thing->setStateValue(cionPowerStateTypeId, false);
//         }
//     });

//     // The maxChargingCurrentE3 takes into account the DIP switches and connected cable, so this is effectively
//     // our maximum. However, it will go to 0 when unplugged, which is odd, so we'll ignore 0 values.
//     connect(cionConnection, &CionModbusRtuConnection::maxChargingCurrentE3Changed, thing, [=](quint16 maxChargingCurrentE3){
//         qCDebug(dcPce()) << "Maximum charging current E3 current changed:" << maxChargingCurrentE3;
//         if (maxChargingCurrentE3 != 0) {
//             thing->setStateMaxValue(cionMaxChargingCurrentStateTypeId, maxChargingCurrentE3);
//         }
//     });

//     connect(cionConnection, &CionModbusRtuConnection::statusBitsChanged, thing, [=](quint16 statusBits){
//         thing->setStateValue(cionConnectedStateTypeId, true);
//         StatusBits status = static_cast<StatusBits>(statusBits);
//         // TODO: Verify if the statusBit for PluggedIn is reliable and if so, use that instead of the plugged in time for the plugged in state.
//         qCDebug(dcPce()) << "Status bits changed:" << status;
//     });

//     connect(cionConnection, &CionModbusRtuConnection::minChargingCurrentChanged, thing, [=](quint16 minChargingCurrent){
//         qCDebug(dcPce()) << "Minimum charging current changed:" << minChargingCurrent;
//         if (minChargingCurrent > 32) {
//             // Apparently this register occationally holds random values... As a quick'n dirty workaround we'll ignore everything > 32
//             qCWarning(dcPce()) << "Detected a bogus min charging current register value (reg. 507) of" << minChargingCurrent << ". Ignoring it...";
//             return;
//         }
//         thing->setStateMinValue(cionMaxChargingCurrentStateTypeId, minChargingCurrent);
//     });

//     connect(cionConnection, &CionModbusRtuConnection::gridVoltageChanged, thing, [=](float /*gridVoltage*/){
// //        qCDebug(dcPce()) << "Grid voltage changed:" << gridVoltage;
//     });

//     connect(cionConnection, &CionModbusRtuConnection::u1VoltageChanged, thing, [=](float /*u1Voltage*/){
// //        qCDebug(dcPce()) << "U1 voltage changed:" << u1Voltage;
//     });

//     connect(cionConnection, &CionModbusRtuConnection::pluggedInDurationChanged, thing, [=](quint32 /*pluggedInDuration*/){
// //        qCDebug(dcPce()) << "Plugged in duration changed:" << pluggedInDuration;
//         // Not reliable to determine if plugged in!
// //        thing->setStateValue(cionPluggedInStateTypeId, pluggedInDuration > 0);
//     });

//     connect(cionConnection, &CionModbusRtuConnection::chargingDurationChanged, thing, [=](quint32 chargingDuration){
// //        qCDebug(dcPce()) << "Charging duration changed:" << chargingDuration;
//         thing->setStateValue(cionChargingStateTypeId, chargingDuration > 0);
//     });

//     cionConnection->update();

//     // Initialize min/max to their defaults. If both, nymea and the wallbox are restarted simultaneously, nymea would cache the min/max while
//     // the wallbox would revert to its defaults, and being the default, the modbusconnection also never emits "changed" signals for them.
//     // To prevent running out of sync we'll "uncache" min/max values too.
//     thing->setStateMinMaxValues(cionMaxChargingCurrentStateTypeId, 6, 32);

//     connect(thing, &Thing::settingChanged, this, [this, thing](const ParamTypeId &paramTypeId, const QVariant &value){
//         if (paramTypeId == cionSettingsPhasesParamTypeId) {
//             qCInfo(dcPce()) << "The connected phases setting has changed to" << value.toString();
//             updatePhaseCount(thing, value.toString());
//         }
//     });

//     updatePhaseCount(thing, thing->setting(cionSettingsPhasesParamTypeId).toString());
}

void IntegrationPluginPce::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)
//     qCDebug(dcPce()) << "Post setup thing" << thing->name();
//     if (!m_refreshTimer) {
//         m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
//         connect(m_refreshTimer, &PluginTimer::timeout, this, [this] {
//             foreach (Thing *thing, myThings()) {

//                 CionModbusRtuConnection *connection = m_connections.value(thing);
//                 connection->update();

//                 // The only apparent way to know whether it is charging, is to compare if lastChargingDuration has changed
//                 // If it didn't change in the last cycle
//                 qulonglong lastChargingDuration = connection->property("lastChargingDuration").toULongLong();
//                 thing->setStateValue(cionChargingStateTypeId, connection->chargingDuration() != lastChargingDuration);
//                 connection->setProperty("lastChargingDuration", connection->chargingDuration());

//                 // Reading the CP signal state to know if the wallbox is reachable.
//                 // Note that this is also an "update" register, hence being read twice.
//                 // We'll not actually evaluate the actual results in here because
//                 // this piece of code should be replaced with the modbus tool internal connected detection when it's ready
//                 ModbusRtuReply *reply = connection->readCpSignalState();
//                 connect(reply, &ModbusRtuReply::finished, thing, [reply, thing, this](){
// //                    qCDebug(dcPce) << "CP signal state reply finished" << reply->error();
//                     thing->setStateValue(cionConnectedStateTypeId, reply->error() == ModbusRtuReply::NoError);

//                     // The Cion seems to crap out rather often and needs to be reconnected :/
//                     if (reply->error() == ModbusRtuReply::TimeoutError) {
//                         QUuid uuid = thing->paramValue(cionThingModbusMasterUuidParamTypeId).toUuid();
//                         hardwareManager()->modbusRtuResource()->getModbusRtuMaster(uuid)->requestReconnect();
//                     }
//                 });
//             }
//         });

//         qCDebug(dcPce()) << "Starting refresh timer...";
//         m_refreshTimer->start();
//     }
}

void IntegrationPluginPce::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)

    qCDebug(dcPce()) << "Thing removed" << thing->name();

    if (m_connections.contains(thing))
        m_connections.take(thing)->deleteLater();

    if (myThings().isEmpty() && m_refreshTimer) {
        qCDebug(dcPce()) << "Stopping reconnect timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginPce::executeAction(ThingActionInfo *info)
{
    // CionModbusRtuConnection *cionConnection = m_connections.value(info->thing());
    // if (info->action().actionTypeId() == cionPowerActionTypeId) {
    //     bool enabled = info->action().paramValue(cionPowerActionPowerParamTypeId).toBool();
    //     int maxChargingCurrent = enabled ? info->thing()->stateValue(cionMaxChargingCurrentStateTypeId).toUInt() : 0;
    //     qCDebug(dcPce()) << "Setting charging enabled:" << (enabled ? 1 : 0) << "(charging current setpoint:" << maxChargingCurrent << ")";

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
    //         qCDebug(dcPce) << "Charging enabled command reply:" << reply->error() << reply->errorString();
    //     });

    //     // And restore the charging current in case setting the above fails
    //     reply = cionConnection->setChargingCurrentSetpoint(maxChargingCurrent);
    //     waitForActionFinish(info, reply, cionPowerStateTypeId, enabled);
    //     connect(reply, &ModbusRtuReply::finished, info, [reply](){
    //         qCDebug(dcPce) << "Implicit max charging current setpoint command reply:" << reply->error() << reply->errorString();
    //     });


    // } else if (info->action().actionTypeId() == cionMaxChargingCurrentActionTypeId) {
    //     // If charging is set to enabled, we'll write the value to the wallbox
    //     uint maxChargingCurrent = info->action().paramValue(cionMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
    //     if (info->thing()->stateValue(cionPowerStateTypeId).toBool()) {
    //         qCDebug(dcPce) << "Charging is enabled. Applying max charging current setpoint of" << maxChargingCurrent << "to wallbox";
    //         ModbusRtuReply *reply = cionConnection->setChargingCurrentSetpoint(maxChargingCurrent);
    //         waitForActionFinish(info, reply, cionMaxChargingCurrentStateTypeId, maxChargingCurrent);
    //         connect(reply, &ModbusRtuReply::finished, info, [reply](){
    //             qCDebug(dcPce) << "Charging current setpoint command reply:" << reply->error() << reply->errorString();
    //         });

    //     } else { // we'll just memorize what the user wants in our state and write it when enabled is set to true
    //         qCDebug(dcPce) << "Charging is disabled, storing max charging current of" << maxChargingCurrent << "to state";
    //         info->thing()->setStateValue(cionMaxChargingCurrentStateTypeId, maxChargingCurrent);
    //         info->finish(Thing::ThingErrorNoError);
    //     }
    // }


    Q_ASSERT_X(false, "IntegrationPluginPce::executeAction", QString("Unhandled action: %1").arg(info->action().actionTypeId().toString()).toLocal8Bit());
}
