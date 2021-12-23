#pragma once

#define SET_AMQP_ORIGIN_FIELD()

namespace enigma::amqp {
	class exchange_imp : public exchange {
		amqp_connection_state_t* _amqp_conn{ nullptr };
		amqp_channel_t* _amqp_channel{ nullptr };
		std::string _name;
	public:
		exchange_imp(
			amqp_connection_state_t* conn,
			amqp_channel_t* channel,
			const std::string& name) : _amqp_conn(conn), _amqp_channel(channel), _name(name) {}
		virtual ~exchange_imp() {}

		virtual inline std::string name() const {
			return _name;
		}

		void declare(
			const std::string& type,
			bool passive,
			bool durable,
			bool auto_delete,
			bool internal,
			argument_table&& args) {
			table_impl table(args);
			auto r = amqp_exchange_declare(
				*_amqp_conn,
				*_amqp_channel,
				_name.empty() ? amqp_empty_bytes : amqp_cstring_bytes(_name.c_str()),
				amqp_cstring_bytes(type.c_str()),
				passive,
				durable,
				auto_delete,
				internal,
				table);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"declare exchange");
		}

		virtual void bind(
			const std::string& destination,
			const std::string& routing_key,
			argument_table&& args = {}) {
			table_impl table(args);
			auto r = amqp_exchange_bind(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				amqp_cstring_bytes(destination.c_str()),
				amqp_cstring_bytes(routing_key.c_str()),
				table
			);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"bind exchange");
		}

		virtual void unbind(
			const std::string& destination,
			const std::string& routing_key,
			argument_table&& args = {}) {
			table_impl table(args);
			auto r = amqp_exchange_unbind(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				amqp_cstring_bytes(destination.c_str()),
				amqp_cstring_bytes(routing_key.c_str()),
				table);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"unbind exchange");
		}

		virtual void del(bool if_unused) {
			amqp_exchange_delete(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				if_unused);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"delete exchange");
		}

		virtual void publish(
			const std::string& routing_key,
			bool mandatory,
			bool immediate,
			const char* body,
			const std::size_t length,
			std::optional<properties> props = std::nullopt) {
			amqp_basic_properties_t_ origin_props;
			table_impl table;
			origin_props._flags = 0;
			if (props) {
				if (props->app_id.length()) {
					origin_props.app_id = amqp_cstring_bytes(props->app_id.c_str());
					origin_props._flags |= AMQP_BASIC_APP_ID_FLAG;
				}
				if (props->cluster_id.length()) {
					origin_props.cluster_id = amqp_cstring_bytes(props->cluster_id.c_str());
					origin_props._flags |= AMQP_BASIC_CLUSTER_ID_FLAG;
				}
				if (props->content_encoding.length()) {
					origin_props.content_encoding = amqp_cstring_bytes(props->content_encoding.c_str());
					origin_props._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
				}
				if (props->content_type.length()) {
					origin_props.content_type = amqp_cstring_bytes(props->content_type.c_str());
					origin_props._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
				}
				if (props->correlation_id.length()) {
					origin_props.correlation_id = amqp_cstring_bytes(props->correlation_id.c_str());
					origin_props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
				}
				if (props->delivery_mode) {
					origin_props.delivery_mode = props->delivery_mode;
					origin_props._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
				}
				if (props->expiration.length()) {
					origin_props.expiration = amqp_cstring_bytes(props->expiration.c_str());
					origin_props._flags |= AMQP_BASIC_EXPIRATION_FLAG;
				}
				if (props->message_id.length()) {
					origin_props.message_id = amqp_cstring_bytes(props->message_id.c_str());
					origin_props._flags |= AMQP_BASIC_MESSAGE_ID_FLAG;
				}
				if (props->reply_to.length()) {
					origin_props.reply_to = amqp_cstring_bytes(props->reply_to.c_str());
					origin_props._flags |= AMQP_BASIC_REPLY_TO_FLAG;
				}
				if (props->type.length()) {
					origin_props.type = amqp_cstring_bytes(props->type.c_str());
					origin_props._flags |= AMQP_BASIC_TYPE_FLAG;
				}
				if (props->user_id.length()) {
					origin_props.user_id = amqp_cstring_bytes(props->user_id.c_str());
					origin_props._flags |= AMQP_BASIC_USER_ID_FLAG;
				}
				if (props->priority) {
					origin_props.priority = props->priority;
					origin_props._flags |= AMQP_BASIC_PRIORITY_FLAG;
				}
				if (props->timestamp) {
					origin_props.timestamp = props->timestamp;
					origin_props._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
				}
				if (props->headers.size()) {
					table.make(props->headers);
					origin_props.headers = table;
					origin_props._flags |= AMQP_BASIC_HEADERS_FLAG;
				}
				else origin_props.headers = amqp_empty_table;
			}

			amqp_bytes_t bytes{ length, (void*)body };
			amqp_reply::check(amqp_basic_publish(
				*_amqp_conn,
				*_amqp_channel,
				_name.empty() ? amqp_empty_bytes : amqp_cstring_bytes(_name.c_str()),
				routing_key.empty() ? amqp_empty_bytes : amqp_cstring_bytes(routing_key.c_str()),
				mandatory,
				immediate,
				props ? &origin_props : nullptr,
				bytes),
				"basic publish");
		}
	};
}
