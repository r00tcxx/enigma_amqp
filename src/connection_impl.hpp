#pragma once
#include <mutex>
#include "amqp.h"
#include "channel_impl.hpp"
#include "exception_impl.hpp"

namespace enigma::amqp {

	class connection_impl {
		amqp_socket_t* _socket{ nullptr };
		amqp_connection_state_t _conn;
		amqp_channel_t _chan;
		std::string _vhost, _username, _password;
		std::string _ipv4;
		std::once_flag _f;
		unsigned short _port{ 0 };
		channel_impl* _channel_imp{ nullptr };
	public:
		void create(
			const std::string& vhost,
			const std::string& username,
			const std::string& password,
			const int heartbeat_interval,
			const std::string& ip,
			unsigned short port) {
			std::call_once(_f, [&] {
				_vhost = vhost, _username = username, _password = password, _ipv4 = ip;
				_port = port;

				_conn = amqp_new_connection();
				_socket = amqp_tcp_socket_new(_conn);
				throw_if(!_socket, "amqp: create tcp socket");
				if (int status = amqp_socket_open(
					_socket,
					ip.c_str(),
					port))
					throw_if(!_socket, "amqp: open tcp socket, status: " + std::to_string(status));

				amqp_reply::check(amqp_login(
					_conn,
					vhost.c_str(),
					0,
					131072,
					heartbeat_interval,
					AMQP_SASL_METHOD_PLAIN,
					_username.c_str(),
					_password.c_str()),
					"login");

				_chan = 1;
				amqp_channel_open(_conn, _chan);
				amqp_reply::check(
					amqp_get_rpc_reply(_conn),
					"open channel");

				_channel_imp = new channel_impl(
					&_conn,
					&_chan);
			});
		}

		channel* get_channel() {
			return _channel_imp;
		}

		void close() {
			amqp_reply::check(amqp_connection_close(
				_conn,
				AMQP_REPLY_SUCCESS),
				"close connection");

			_channel_imp->close();

			amqp_reply::check(
				amqp_destroy_connection(_conn),
				"destory connection");
		}
	};




}
