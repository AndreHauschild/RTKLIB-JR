QT += widgets core gui

include(../qtapp.pri)

TEMPLATE = app
TARGET = rtkconv_qt
target.path = $$INSTALLROOT/bin
INSTALLS += target

INCLUDEPATH += ../../../src/ ../appcmn_qt

SOURCES += \ 
    codeopt.cpp \
    convmain.cpp \
    convopt.cpp \
    rtkconv.cpp \
    startdlg.cpp \
    ../appcmn_qt/aboutdlg.cpp \
    ../appcmn_qt/keydlg.cpp \
    ../appcmn_qt/timedlg.cpp \
    ../appcmn_qt/viewer.cpp \
    ../appcmn_qt/vieweropt.cpp \
    ../appcmn_qt/glofcndlg.cpp \
    ../appcmn_qt/mntpoptdlg.cpp \
    ../appcmn_qt/helper.cpp \
    ../appcmn_qt/freqdlg.cpp

HEADERS  += \ 
    codeopt.h \
    convmain.h \
    convopt.h \
    startdlg.h \
    ../appcmn_qt/aboutdlg.h \
    ../appcmn_qt/keydlg.h \
    ../appcmn_qt/timedlg.h \
    ../appcmn_qt/viewer.h \
    ../appcmn_qt/vieweropt.h \
    ../appcmn_qt/glofcndlg.h \
    ../appcmn_qt/mntpoptdlg.h \
    ../appcmn_qt/helper.h \
    ../appcmn_qt/freqdlg.h

FORMS    += \
    codeopt.ui \
    convopt.ui \
    startdlg.ui \
    convmain.ui \
    ../appcmn_qt/aboutdlg.ui \
    ../appcmn_qt/keydlg.ui \
    ../appcmn_qt/timedlg.ui \
    ../appcmn_qt/viewer.ui \
    ../appcmn_qt/vieweropt.ui \
    ../appcmn_qt/glofcndlg.ui \
    ../appcmn_qt/mntpoptdlg.ui \
    ../appcmn_qt/freqdlg.ui

RESOURCES += \
    ../appcmn_qt/appcmn_qt.qrc \
    ../icon/resources.qrc

RC_FILE = rtkconv_qt.rc

desktop.path   = $$INSTALLROOT/share/applications/
desktop.files += rtkconv_qt.desktop
INSTALLS      += desktop

icons.path   = $$INSTALLROOT/share/pixmaps/
icons.files += ../icon/rtkconv.png
INSTALLS    += icons

win32 {
CONFIG(release,debug|release) {
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OUT_PWD/release/$${TARGET}.exe) $$shell_path($$PKGDIR/packages/com.rtklib.$${TARGET}/data)
}
}
