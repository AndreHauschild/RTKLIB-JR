#-------------------------------------------------
#
# Project created by QtCreator 2016-02-09T19:09:57
#
#-------------------------------------------------

QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 5) {  # QT 6
    QT += quickwidgets
}

include(../qtapp.pri)

qtHaveModule(webenginewidgets) {
    QT+= webenginewidgets
    DEFINES+=QWEBENGINE
}

INCLUDEPATH += ../../../src/ ../appcmn_qt

TEMPLATE = app

SOURCES += \ 
    browsmain.cpp \
    srctblbrows.cpp \
    staoptdlg.cpp \
    ../appcmn_qt/mapview.cpp \
    ../appcmn_qt/mapviewopt.cpp \
    ../appcmn_qt/aboutdlg.cpp \
    ../appcmn_qt/helper.cpp \
    ../appcmn_qt/viewer.cpp \
    ../appcmn_qt/vieweropt.cpp

HEADERS  += \ 
    browsmain.h \
    staoptdlg.h \
    ../appcmn_qt/mapview.h \
    ../appcmn_qt/mapviewopt.h \
    ../appcmn_qt/gm_template.h \
    ../appcmn_qt/aboutdlg.h \
    ../appcmn_qt/helper.h \
    ../appcmn_qt/viewer.h \
    ../appcmn_qt/vieweropt.h

FORMS    += \ 
    browsmain.ui \
    staoptdlg.ui \
    ../appcmn_qt/mapview.ui \
    ../appcmn_qt/mapviewopt.ui \
    ../appcmn_qt/aboutdlg.ui \
    ../appcmn_qt/viewer.ui \
    ../appcmn_qt/vieweropt.ui

RESOURCES +=  \
    srctblbrows_qt.qrc \
    ../appcmn_qt/appcmn_qt.qrc

RC_FILE = srctblbrows_qt.rc
