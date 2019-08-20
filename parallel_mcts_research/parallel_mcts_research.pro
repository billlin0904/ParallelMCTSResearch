TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp\
           ./websocket/websocket.cpp\
           ./websocket/websocket_server.cpp\

INCLUDEPATH += ./thirdparty/rapidjson/include/

linux-g++* {
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -std=gnu++11 -g
} else {
    QMAKE_CXXFLAGS += -std=gnu++11 -O3
}

INCLUDEPATH += /home/rdbill0452/libs/boost_1_70_0/ \

LIBS += -L/home/rdbill0452/libs/boost_1_70_0/stage/lib \
    -lboost_system \
    -lboost_filesystem \
    -lboost_thread \

LIBS += -lssl -lcrypto -lpthread \
}

HEADERS += \
    rng.h \
    node.h \
    ./websocket/websocket.h \
    ./websocket/websocket_server.h \
    mcts.h \
    websocket/websocket_server.h
