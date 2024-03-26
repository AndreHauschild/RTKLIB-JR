QT += core gui widgets concurrent

include(../qtapp.pri)

qtHaveModule(webenginewidgets) {
    QT += webenginewidgets
    DEFINES += QWEBENGINE
}

INCLUDEPATH += ../../../src/ ../appcmn_qt

TEMPLATE = app
TARGET = srctblbrows_qt
target.path = $$INSTALLROOT/bin
INSTALLS += target

SOURCES += \ 
    browsmain.cpp \
    srctblbrows.cpp \
    ../appcmn_qt/staoptdlg.cpp \
    ../appcmn_qt/mapview.cpp \
    ../appcmn_qt/mapviewopt.cpp \
    ../appcmn_qt/aboutdlg.cpp \
    ../appcmn_qt/helper.cpp \
    ../appcmn_qt/mntpoptdlg.cpp \
    ../appcmn_qt/viewer.cpp \
    ../appcmn_qt/vieweropt.cpp

HEADERS += \
    browsmain.h \
    ../appcmn_qt/staoptdlg.h \
    ../appcmn_qt/mapview.h \
    ../appcmn_qt/mapviewopt.h \
    ../appcmn_qt/gm_template.h \
    ../appcmn_qt/aboutdlg.h \
    ../appcmn_qt/helper.h \
    ../appcmn_qt/mntpoptdlg.h \
    ../appcmn_qt/viewer.h \
    ../appcmn_qt/vieweropt.h

FORMS    += \ 
    browsmain.ui \
    ../appcmn_qt/staoptdlg.ui \
    ../appcmn_qt/mapview.ui \
    ../appcmn_qt/mapviewopt.ui \
    ../appcmn_qt/aboutdlg.ui \
    ../appcmn_qt/mntpoptdlg.ui \
    ../appcmn_qt/viewer.ui \
    ../appcmn_qt/vieweropt.ui

RESOURCES += \
    ../appcmn_qt/appcmn_qt.qrc \
    ../icon/resources.qrc

RC_FILE = srctblbrows_qt.rc

desktop.path   = $$INSTALLROOT/share/applications/
desktop.files += srctblbrows_qt.desktop
INSTALLS      += desktop

icons.path   = $$INSTALLROOT/share/pixmaps/
icons.files += ../icon/srctblbrows.png
INSTALLS    += icons

win32 {
CONFIG(release,debug|release) {
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OUT_PWD/release/$${TARGET}.exe) $$shell_path($$PKGDIR/packages/com.rtklib.$${TARGET}/data)
}
}
