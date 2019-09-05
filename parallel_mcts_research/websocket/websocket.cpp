#include "websocket.h"
#include <boost/stacktrace.hpp>

namespace websocket {

Exception::Exception(boost::system::error_code ec) {
    std::ostringstream ostr;
	ostr << "Category: " << ec.category().name() << " (" << ec.value() << ") " << ec.message()
		<< "\r\n"
		<< "Stack trace:\r\n"
		<< boost::stacktrace::stacktrace();
    message_ = ostr.str();
}

const char* Exception::what() const noexcept {
    return message_.c_str();
}

}
