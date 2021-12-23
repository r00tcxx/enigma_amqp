#pragma once
#include "table_impl.hpp"

namespace enigma::amqp {
	class message_impl : public message {
		std::shared_ptr<amqp_envelope_t> _origin_envelopt{ nullptr };
		std::shared_ptr<amqp_message_t> _origin_msg { nullptr };
		properties _props;
		std::uint64_t _delivery_tag{ 0 };

		bool _requeue{ false }, _ack{ true }, _reject{ false }, _fair{ false };
	public:
		message_impl() {}

		virtual ~message_impl() {
			if (!_origin_envelopt && !_origin_msg)
				return;
			if (_fair) 
				amqp_destroy_message(_origin_msg.get());
			else 
				amqp_destroy_envelope(_origin_envelopt.get());
		}

		inline void init(bool fair) {
			_fair = fair;
			if (_fair) _origin_msg = std::make_shared<amqp_message_t>();
			else _origin_envelopt = std::make_shared<amqp_envelope_t>();
		}

		inline amqp_envelope_t* get_envelopt() {
			return _origin_envelopt.get();
		}

		inline amqp_message_t* get_origin_message() {
			return _origin_msg.get();
		}

		inline void set_delivery_tag(std::uint64_t tag) {
			_delivery_tag = tag;
		}

		inline std::uint64_t get_delivery_tag() const {
			return _delivery_tag;
		}

		virtual properties get_properties() {
			return _props;
		}

		virtual const char* get_body() const {
			return _fair ? (const char*)_origin_msg->body.bytes :
				(const char*) _origin_envelopt->message.body.bytes;
		}

		virtual const long long get_length() const {
			return _fair ? _origin_msg->body.len :
				_origin_envelopt->message.body.len;
		}

		virtual void reject(bool requeue) {
			_reject = true, _requeue = requeue;
		}

		virtual void ack(bool ack) {
			_ack = ack;
		}

		virtual void requeue(bool requeue) {
			_requeue = requeue;
		}

		virtual bool is_ack() const {
			return _ack;
		}

		virtual bool is_reject() const {
			return _reject;
		}

		virtual bool is_requeue() const {
			return _requeue;
		}
	};
}
