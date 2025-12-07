QT += core widgets printsupport
CONFIG += c++17
TARGET = RepairShopApp
TEMPLATE = app

SOURCES += \
    clientnotifier.cpp \
    main.cpp \
    mainwindow.cpp \
    loginwindow.cpp \
    modelmanager.cpp \
    user.cpp \
    device.cpp \
    part.cpp \
    repairorder.cpp \
    statustracker.cpp \
    statusstrategy.cpp \
    orderobserver.cpp \
    datastorage.cpp

HEADERS += \
    clientnotifier.h \
    mainwindow.h \
    loginwindow.h \
    modelmanager.h \
    user.h \
    device.h \
    part.h \
    repairorder.h \
    statustracker.h \
    statusstrategy.h \
    orderobserver.h \
    datastorage.h

FORMS += \
    mainwindow.ui \
    loginwindow.ui
