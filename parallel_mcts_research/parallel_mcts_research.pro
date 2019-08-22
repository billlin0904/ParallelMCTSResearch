TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp\
            ./websocket/websocket.cpp\
           ./websocket/websocket_client.cpp\
           ./websocket/websocket_server.cpp\

INCLUDEPATH += ./thirdparty/rapidjson/include/

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
    ./websocket/websocket_client.h \
    ./websocket/websocket_server.h \
    mcts.h \
    websocket/websocket_server.h \
    websocket/websocket.h
