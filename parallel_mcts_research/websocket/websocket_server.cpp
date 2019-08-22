#include "websocket_server.h"

namespace websocket {

int32_t NewSessionID() noexcept {
    static std::atomic<int32_t> sSessionID(10000);
    return ++sSessionID;
}

}
