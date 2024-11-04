QT += widgets core gui

CONFIG += c++11

# save root directory
ROOT_DIRECTORY = $${PWD}/../../..

include(../../../RTKLib.pri)

TEMPLATE = app
TARGET = rtklaunch_qt
target.path = $$INSTALLROOT/bin
INSTALLS += target

INCLUDEPATH += ../../../src/

SOURCES += \
    launchmain.cpp \
    main.cpp \
    launchoptdlg.cpp

HEADERS  += \
    launchmain.h \
    launchoptdlg.h

FORMS    += \
    launchmain.ui \
    launchoptdlg.ui

RESOURCES += \
    ../appcmn_qt/appcmn_qt.qrc \
    ../icon/resources.qrc

RC_FILE = rtklaunch_qt.rc

desktop.path   = $$INSTALLROOT/share/applications/
desktop.files += rtklaunch_qt.desktop
INSTALLS      += desktop

icons.path   = $$INSTALLROOT/share/pixmaps/
icons.files += ../icon/rtklaunch.png
INSTALLS    += icons

win32 {
CONFIG(release,debug|release) {
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OUT_PWD/release/$${TARGET}.exe) $$shell_path($$PKGDIR/packages/com.rtklib.$${TARGET}/data)
}
}
