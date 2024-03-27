include(../../RTKLib.pri)

# save root directory
ROOT_DIRECTORY = $${PWD}/../..
OUTPUT_DIRECTORY = $${OUT_PWD}

QMAKE_LIBDIR += ../../../lib
QMAKE_LIBDIR += $${ROOT_DIRECTORY}/lib
QMAKE_LIBDIR += $${ROOT_DIRECTORY}/src

LIBS += -L$${ROOT_DIRECTORY}/lib/ -lRTKLib

IERS_MODEL {
    LIBS += -liers -lgfortran
}

win* {
    LIBS += -lws2_32 -lwinmm
}

QMAKE_RPATHDIR *= $${ROOT_DIRECTORY}/lib

PRE_TARGETDEPS = $${ROOT_DIRECTORY}/src/rtklib.h
