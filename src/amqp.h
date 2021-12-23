#pragma once
#include <string>
#include <memory>
#include <map>
#include <memory>
#include <variant>
#include <functional>
#include <optional>

namespace enigma::amqp {
	template<typename _Ty>
	struct amqp_type { _Ty value; };

	using table_value = std::variant<
		amqp_type<bool>,
		amqp_type<float>,
		amqp_type<double>,
		amqp_type<std::int8_t>,
		amqp_type<std::int16_t>,
		amqp_type<std::int32_t>,
		amqp_type<std::int64_t>,
		std::string>;

	using argument_table = std::map<std::string, table_value>;

	struct properties {
		std::string content_type;
		std::string content_encoding;
		argument_table headers;
		std::uint8_t delivery_mode{ 0 };
		std::uint8_t priority{ 0 };
		std::string correlation_id;
		std::string reply_to;
		std::string expiration;
		std::string message_id;
		std::uint64_t timestamp{ 0 };
		std::string type;
		std::string user_id;
		std::string app_id;
		std::string cluster_id;
	};

	class message {
	public:
		message() {}
		virtual ~message() {}

		virtual properties get_properties() = 0;
		virtual const char* get_body() const = 0;
		virtual const long long get_length() const = 0;
		virtual bool is_ack() const = 0;
		virtual bool is_reject() const = 0;
		virtual bool is_requeue() const = 0;

		virtual void requeue(bool) = 0;
		virtual void reject(bool requeue) = 0;
		virtual void ack(bool ack) = 0;
	};

	class queue {
	public:
		queue() {}
		virtual ~queue() {}

		virtual std::string name() const = 0;

		virtual void bind(
			const std::string& exchange_name,
			const std::string& routing_key,
			argument_table&& args = {}) = 0;

		virtual void unbind(
			const std::string& exchange_name,
			const std::string& routing_key,
			argument_table&& args = {}) = 0;

		virtual void del(
			bool if_unused,
			bool if_empty) = 0;

		virtual void purge() = 0;

		virtual void using_basic_consume(
			const std::string& consumer_tag,
			bool no_local,
			bool exclusive,
			argument_table&& args = {}) = 0;

		virtual void using_fair_consume() = 0;

		virtual void consume(
			bool auto_ack,
			std::function<void(message* msg)> cb,
			std::uint32_t timeout_s = 0) = 0;

		virtual void publish(
			bool mandatory,
			bool immediate,
			const char* body,
			const std::size_t length,
			std::optional<properties> props = std::nullopt) = 0;
	};

	class exchange {
	public:
		exchange() {}
		virtual ~exchange() {}

		virtual std::string name() const = 0;

		virtual void bind(
			const std::string& destination,
			const std::string& routing_key,
			argument_table&& args = {}) = 0;

		virtual void unbind(
			const std::string& destination,
			const std::string& routing_key,
			argument_table&& args = {}) = 0;

		virtual void del(bool if_unused) = 0;

		virtual void publish(
			const std::string& routing_key,
			bool mandatory,
			bool immediate,
			const char* body,
			const std::size_t length,
			std::optional<properties> props = std::nullopt) = 0;
	};


	class channel {
	public:
		channel() {}
		virtual ~channel() {}

		virtual void close() = 0;

		virtual queue* declare_queue(
			const std::string& queue_name,
			bool passive,
			bool durable,
			bool exclusive,
			bool auto_delete,
			argument_table&& args = {}) = 0;

		virtual exchange* declare_exchange(
			const std::string& exchange_name,
			const std::string& type,
			bool passive,
			bool durable,
			bool auto_delete,
			bool internal,
			argument_table&& args = {}) = 0;
	};

	class connection_impl;
	class connection {
		connection_impl* _impl{ nullptr };
		channel* _channel{ nullptr };
	public:
		connection();
		virtual ~connection();

		virtual void create(
			const std::string& vhost,
			const std::string& username,
			const std::string& password,
			const int heartbeat_interval,
			const std::string& ip,
			unsigned short port = 5672);

		virtual channel* get_channel();

		virtual void close();
	};
}

