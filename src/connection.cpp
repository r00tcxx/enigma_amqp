#include <mutex>
#include "amqp.h"
#include "connection_impl.hpp"
#include "exception_impl.hpp"

namespace enigma::amqp {
	connection::connection() {
		_impl = new connection_impl;
	}

	connection::~connection() {
		delete _impl; _impl = nullptr;
	}

	void connection::create(
		const std::string& vhost,
		const std::string& username,
		const std::string& password,
		const int heartbeat_interval,
		const std::string& ip,
		unsigned short port) {

		_impl->create(
			vhost,
			username,
			password,
			heartbeat_interval,
			ip,
			port);
	}

	channel* connection::get_channel() {
		return _impl->get_channel();
	}

	void connection::close() {
		if (_impl)
			_impl->close();
	}
}
