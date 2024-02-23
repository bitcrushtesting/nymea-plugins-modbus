include(../plugins.pri)

# Generate modbus connection
MODBUS_CONNECTIONS += EV11.3-registers.json
#MODBUS_TOOLS_CONFIG += VERBOSE
include(../modbus.pri)

HEADERS += \
    ev11wallbox.h \
    integrationpluginpcelectric.h \
    pcelectricdiscovery.h

SOURCES += \
    ev11wallbox.cpp \
    integrationpluginpcelectric.cpp \
    pcelectricdiscovery.cpp

