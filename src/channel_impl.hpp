#pragma once
#include "amqp.h"
#include "queue_impl.hpp"
#include "exchange_imp.hpp"
#include "exception_impl.hpp"

namespace enigma::amqp {
	class channel_impl : public channel {
		amqp_connection_state_t* _amqp_conn{ nullptr };
		amqp_channel_t* _amqp_channel{ nullptr };
		std::map<std::string, std::shared_ptr<queue_impl>> _queues;
		std::map<std::string, std::shared_ptr<exchange_imp>> _exchanges;
	public:
		channel_impl(
			amqp_connection_state_t* conn,
			amqp_channel_t* channel) : _amqp_conn(conn), _amqp_channel(channel) {}

		virtual ~channel_impl() {}

		virtual queue* declare_queue(
			const std::string& queue_name,
			bool passive,
			bool durable,
			bool exclusive,
			bool auto_delete,
			argument_table&& args) {

			auto match = _queues.find(queue_name);
			if (match == _queues.end()) {
				auto ptr = std::make_shared<queue_impl>(
					_amqp_conn,
					_amqp_channel,
					queue_name);
				_queues[queue_name] = ptr; 

				ptr->declare(
					passive,
					durable,
					exclusive,
					auto_delete,
					std::move(args));
				return ptr.get();
			}
			return dynamic_cast<queue*>(match->second.get());
		}

		virtual exchange* declare_exchange(
			const std::string& exchange_name,
			const std::string& type,
			bool passive,
			bool durable,
			bool auto_delete,
			bool internal,
			argument_table&& args = {}) {
			auto match = _exchanges.find(exchange_name);
			if (match == _exchanges.end()) {
				auto ptr = std::make_shared<exchange_imp>(
					_amqp_conn,
					_amqp_channel,
					exchange_name);
				_exchanges[exchange_name] = ptr;

				ptr->declare(
					type,
					passive,
					durable,
					auto_delete,
					internal,
					std::move(args));
				return ptr.get();
			}
			return dynamic_cast<exchange*>(match->second.get());
		}

		virtual void close() {
			amqp_reply::check(
				amqp_channel_close(
					*_amqp_conn,
					*_amqp_channel,
					AMQP_REPLY_SUCCESS),
				"close channel");
		}
	};
}
