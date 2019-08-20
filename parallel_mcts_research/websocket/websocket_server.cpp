#include "websocket_server.h"

namespace websocket {

Exception::Exception(boost::system::error_code ec) {
	std::ostringstream ostr;
	ostr << ec.category().name() << ':' << ec.value() << " " << ec.message();
	message_ = ostr.str();
}

const char* Exception::what() const {
	return message_.c_str();
}

}