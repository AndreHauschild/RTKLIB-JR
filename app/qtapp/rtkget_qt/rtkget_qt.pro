QT += core gui widgets

include(../qtapp.pri)

INCLUDEPATH += ../../../src/ ../appcmn_qt

TEMPLATE = app
TARGET = rtkget_qt
target.path = $$INSTALLROOT/bin
INSTALLS += target

SOURCES += main.cpp \
    getmain.cpp \
    getoptdlg.cpp \
    ../appcmn_qt/staoptdlg.cpp \
    ../appcmn_qt/aboutdlg.cpp \
    ../appcmn_qt/keydlg.cpp \
    ../appcmn_qt/viewer.cpp \
    ../appcmn_qt/vieweropt.cpp \
    ../appcmn_qt/helper.cpp \
    ../appcmn_qt/timedlg.cpp

HEADERS  += \
    getmain.h \
    getoptdlg.h \
    ../appcmn_qt/staoptdlg.h \
    ../appcmn_qt/aboutdlg.h \
    ../appcmn_qt/keydlg.h \
    ../appcmn_qt/viewer.h \
    ../appcmn_qt/vieweropt.h \
    ../appcmn_qt/helper.h \
    ../appcmn_qt/timedlg.h

FORMS    += \
    getmain.ui \
    getoptdlg.ui \
    ../appcmn_qt/staoptdlg.ui \
    ../appcmn_qt/aboutdlg.ui \
    ../appcmn_qt/keydlg.ui \
    ../appcmn_qt/viewer.ui \
    ../appcmn_qt/vieweropt.ui \
    ../appcmn_qt/timedlg.ui

RESOURCES += \
    ../appcmn_qt/appcmn_qt.qrc \
    ../icon/resources.qrc

RC_FILE = rtkget_qt.rc

desktop.path   = $$INSTALLROOT/share/applications/
desktop.files += rtkget_qt.desktop
INSTALLS      += desktop

icons.path   = $$INSTALLROOT/share/pixmaps/
icons.files += ../icon/rtkget.png
INSTALLS    += icons

win32 {
CONFIG(release,debug|release) {
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OUT_PWD/release/$${TARGET}.exe) $$shell_path($$PKGDIR/packages/com.rtklib.$${TARGET}/data)
}
}
