include(../plugins.pri)

# Generate modbus connection
MODBUS_CONNECTIONS += EV11.3-registers.json
#MODBUS_TOOLS_CONFIG += VERBOSE
include(../modbus.pri)

HEADERS += \
    integrationpluginpcelectric.h \
    pcelectricdiscovery.h

SOURCES += \
    integrationpluginpcelectric.cpp \
    pcelectricdiscovery.cpp

