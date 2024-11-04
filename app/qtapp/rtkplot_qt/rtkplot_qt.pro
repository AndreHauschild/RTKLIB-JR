QT += widgets core gui xml serialport

qtHaveModule(webenginewidgets) {
    QT += webenginewidgets
    DEFINES +=QWEBENGINE
} else {
    qtHaveModule(webkitwidgets) {
        QT += webkitwidgets
        DEFINES += QWEBKIT
    }
}
include(../qtapp.pri)

TEMPLATE = app
TARGET = rtkplot_qt
target.path = $$INSTALLROOT/bin
INSTALLS += target

INCLUDEPATH += ../../../src/ ../appcmn_qt

SOURCES += \
    conndlg.cpp \
    mapoptdlg.cpp \
    plotcmn.cpp \
    plotdata.cpp \
    plotdraw.cpp \
    plotinfo.cpp \
    plotmain.cpp \
    plotopt.cpp \
    pntdlg.cpp \
    rtkplot.cpp \
    skydlg.cpp \
    fileseldlg.cpp \
    vmapdlg.cpp \
    ../appcmn_qt/aboutdlg.cpp \
    ../appcmn_qt/refdlg.cpp \
    ../appcmn_qt/freqdlg.cpp \
    ../appcmn_qt/viewer.cpp \
    ../appcmn_qt/vieweropt.cpp \
    ../appcmn_qt/cmdoptdlg.cpp \
    ../appcmn_qt/fileoptdlg.cpp \
    ../appcmn_qt/serioptdlg.cpp \
    ../appcmn_qt/tcpoptdlg.cpp \
    ../appcmn_qt/mntpoptdlg.cpp \
    ../appcmn_qt/keydlg.cpp \
    ../appcmn_qt/graph.cpp \
    ../appcmn_qt/console.cpp \
    ../appcmn_qt/tspandlg.cpp \
    ../appcmn_qt/timedlg.cpp \
    ../appcmn_qt/helper.cpp \
    ../appcmn_qt/mapview.cpp \
    ../appcmn_qt/mapviewopt.cpp

HEADERS  += \
    conndlg.h \
    mapoptdlg.h \
    plotmain.h \
    plotopt.h \
    pntdlg.h \
    skydlg.h \
    fileseldlg.h \
    vmapdlg.h \
    ../appcmn_qt/aboutdlg.h \
    ../appcmn_qt/refdlg.h \
    ../appcmn_qt/freqdlg.h \
    ../appcmn_qt/viewer.h \
    ../appcmn_qt/vieweropt.h \
    ../appcmn_qt/cmdoptdlg.h \
    ../appcmn_qt/fileoptdlg.h \
    ../appcmn_qt/serioptdlg.h \
    ../appcmn_qt/tcpoptdlg.h \
    ../appcmn_qt/mntpoptdlg.h \
    ../appcmn_qt/keydlg.h \
    ../appcmn_qt/graph.h \
    ../appcmn_qt/console.h \
    ../appcmn_qt/tspandlg.h \
    ../appcmn_qt/timedlg.h \
    ../appcmn_qt/helper.h \
    ../appcmn_qt/mapview.h \
    ../appcmn_qt/mapviewopt.h

FORMS    += \
    conndlg.ui \
    mapoptdlg.ui \
    plotmain.ui \
    plotopt.ui \
    pntdlg.ui \
    skydlg.ui \
    fileseldlg.ui \
    vmapdlg.ui \
    ../appcmn_qt/aboutdlg.ui \
    ../appcmn_qt/refdlg.ui \
    ../appcmn_qt/freqdlg.ui \
    ../appcmn_qt/viewer.ui \
    ../appcmn_qt/vieweropt.ui \
    ../appcmn_qt/cmdoptdlg.ui \
    ../appcmn_qt/fileoptdlg.ui \
    ../appcmn_qt/serioptdlg.ui \
    ../appcmn_qt/tcpoptdlg.ui \
    ../appcmn_qt/mntpoptdlg.ui \
    ../appcmn_qt/keydlg.ui \
    ../appcmn_qt/console.ui \
    ../appcmn_qt/tspandlg.ui \
    ../appcmn_qt/timedlg.ui \
    ../appcmn_qt/mapview.ui \
    ../appcmn_qt/mapviewopt.ui

RESOURCES += \
    ../appcmn_qt/appcmn_qt.qrc \
    ../icon/resources.qrc

RC_FILE = rtkplot_qt.rc

desktop.path   = $$INSTALLROOT/share/applications/
desktop.files += rtkplot_qt.desktop
INSTALLS      += desktop

icons.path   = $$INSTALLROOT/share/pixmaps/
icons.files += ../icon/rtkplot.png
INSTALLS    += icons

win32 {
CONFIG(release,debug|release) {
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OUT_PWD/release/$${TARGET}.exe) $$shell_path($$PKGDIR/packages/com.rtklib.$${TARGET}/data)
}
}
