QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11 silent

TARGET = drawAPNG
TEMPLATE = app

unix: {
	QMAKE_CFLAGS += -Wall -Wextra -pedantic
	QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
#	QMAKE_CFLAGS_RELEASE = -O2 -flto
#	QMAKE_CXXFLAGS_RELEASE = -O2 -flto
#	QMAKE_LFLAGS_RELEASE = -O2 -flto
}
msvc*: {
	CONFIG += embed_manifest_exe
	QMAKE_CFLAGS += -W3
	QMAKE_CXXFLAGS += -W3

	DEFINES += _WINDOWS WIN32
}

INCLUDEPATH += ../
LIBS += -L../ -lAPNG -Wl,-rpath,.

SOURCES += drawAPNG.cxx

HEADERS += drawAPNG.hxx

FORMS += drawAPNG.ui
