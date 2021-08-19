TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
           rng.cpp


INCLUDEPATH += ./thirdparty/spdlog/include/
INCLUDEPATH += /games

linux-g++* {
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -std=gnu++11 -g
    DEFINES += _DEBUG
} else {
    QMAKE_CXXFLAGS += -std=gnu++11
    QMAKE_CXXFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE *= -O3
}
}

HEADERS += \
    rng.h \
    node.h \
    mcts.h \
    threadpool.h \
    games\gomoku\gamestate.h \
    games\tictactoe\gamestate.h \
