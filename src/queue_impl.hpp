#pragma once
#include <WinSock2.h>
#include "message_impl.hpp"
#include "exception_impl.hpp"

namespace enigma::amqp {
	class queue_impl : public queue {
		amqp_connection_state_t* _amqp_conn{ nullptr };
		amqp_channel_t* _amqp_channel{ nullptr };
		std::string _name;
		bool _fair_mod{ false }, _no_ack = true;
	public:
		queue_impl(
			amqp_connection_state_t* conn,
			amqp_channel_t* channel,
			const std::string& name) : _amqp_conn(conn), _amqp_channel(channel), _name(name) {}
		virtual ~queue_impl() {}

		inline std::string name() const {
			return _name;
		}

		void declare(
			bool passive,
			bool durable,
			bool exclusive,
			bool auto_delete,
			argument_table&& args) {
			table_impl table(args);
			auto r = amqp_queue_declare(
				*_amqp_conn,
				*_amqp_channel,
				_name.empty() ? amqp_empty_bytes : amqp_cstring_bytes(_name.c_str()),
				passive,
				durable,
				exclusive,
				auto_delete,
				table);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"declare queue");

			if (_name.empty())		//random name
				_name = (char*)amqp_bytes_malloc_dup(r->queue).bytes;
		}

		virtual void bind(
			const std::string& exchange_name,
			const std::string& routing_key,
			argument_table&& args) {
			table_impl table(args);
			auto r = amqp_queue_bind(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				amqp_cstring_bytes(exchange_name.c_str()),
				amqp_cstring_bytes(routing_key.c_str()),
				table
			);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"bind queue");
		}

		virtual void unbind(
			const std::string& exchange_name,
			const std::string& routing_key,
			argument_table&& args) {
			table_impl table(args);
			amqp_queue_unbind(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				amqp_cstring_bytes(exchange_name.c_str()),
				amqp_cstring_bytes(routing_key.c_str()),
				table);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"unbind queue");
		}

		virtual void del(
			bool if_unused,
			bool if_empty) {
			amqp_queue_delete(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				if_unused,
				if_empty);

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"delete queue");
		}

		virtual void purge() {
			amqp_queue_purge(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()));

			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"purge queue");
		}

		virtual void using_basic_consume(
			const std::string& consumer_tag,
			bool no_local,
			bool exclusive,
			argument_table&& args = {}) {
			table_impl table(args);
			amqp_basic_consume(
				*_amqp_conn,
				*_amqp_channel,
				amqp_cstring_bytes(_name.c_str()),
				amqp_cstring_bytes(consumer_tag.c_str()),
				no_local,
				false,
				exclusive,
				table);
			amqp_reply::check(
				amqp_get_rpc_reply(*_amqp_conn),
				"basic consume ");
			_fair_mod = false;;
		};


		virtual void using_fair_consume() {
			_fair_mod = true;
		}

		virtual void consume(
			bool auto_ack,
			std::function<void(message* msg)> cb,
			std::uint32_t timeout_s) {
			message_impl msg;
			if (_fair_mod) {
				amqp_maybe_release_buffers(*_amqp_conn);
				auto r = amqp_basic_get(
					*_amqp_conn,
					*_amqp_channel,
					amqp_cstring_bytes(_name.c_str()),
					auto_ack);
				if (AMQP_RESPONSE_NORMAL == r.reply_type) {
					if (AMQP_BASIC_GET_EMPTY_METHOD == r.reply.id) {
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						return;
					} 
					else if (r.reply.id == AMQP_BASIC_GET_OK_METHOD) {
						msg.init(true);
						msg.set_delivery_tag((((amqp_basic_get_ok_t*)r.reply.decoded)->delivery_tag));
						amqp_read_message(
							*_amqp_conn,
							*_amqp_channel,
							msg.get_origin_message(),
							0);
					}
				}
				else amqp_reply::check(r, "basic get");
			}
			else {
				amqp_maybe_release_buffers(*_amqp_conn);
				timeval val;
				if (timeout_s)
					val.tv_sec = timeout_s;
				msg.init(false);
				msg.set_delivery_tag(msg.get_envelopt()->delivery_tag);
				amqp_reply::check(
					amqp_consume_message(
						*_amqp_conn,
						msg.get_envelopt(),
						timeout_s ? &val : nullptr,
						auto_ack),
					"consume message");
			}
			if (cb) {
				cb(&msg);
				if (msg.is_reject()) {
					auto code = amqp_basic_reject(
						*_amqp_conn,
						*_amqp_channel,
						msg.get_delivery_tag(),
						msg.is_requeue());
					amqp_reply::check(
						code,
						"reject message");
					return;
				}
				if (auto_ack)  return;
				else if (msg.is_ack()) {
					auto code = amqp_basic_ack(
						*_amqp_conn,
						*_amqp_channel,
						msg.get_delivery_tag(),
						false);

					amqp_reply::check(
						code,
						"basic ack");
				}
				else {
					amqp_reply::check(amqp_basic_nack(
						*_amqp_conn,
						*_amqp_channel,
						msg.get_delivery_tag(),
						false,
						msg.is_requeue()),
						"nack message");
				}
			}
		}

		virtual void publish(
			bool mandatory,
			bool immediate,
			const char* body,
			const std::size_t length,
			std::optional<properties> props) {

			amqp_basic_properties_t_ origin_props;
			table_impl table;
			if (props) {
				origin_props.app_id = amqp_cstring_bytes(props->app_id.c_str());
				origin_props.cluster_id = amqp_cstring_bytes(props->cluster_id.c_str());
				origin_props.content_encoding = amqp_cstring_bytes(props->content_encoding.c_str());
				origin_props.content_type = amqp_cstring_bytes(props->content_type.c_str());
				origin_props.correlation_id = amqp_cstring_bytes(props->correlation_id.c_str());
				origin_props.delivery_mode = props->delivery_mode;
				origin_props.expiration = amqp_cstring_bytes(props->expiration.c_str());
				origin_props.message_id = amqp_cstring_bytes(props->message_id.c_str());
				origin_props.priority = props->priority;
				origin_props.reply_to = amqp_cstring_bytes(props->reply_to.c_str());
				origin_props.timestamp = props->timestamp;
				origin_props.type = amqp_cstring_bytes(props->type.c_str());
				origin_props.user_id = amqp_cstring_bytes(props->user_id.c_str());

				table.make(props->headers);
				origin_props.headers = props->headers.size() ? table : amqp_empty_table;
			}

			amqp_bytes_t bytes{ length, (void*)body };
			amqp_reply::check(amqp_basic_publish(
				*_amqp_conn,
				*_amqp_channel,
				amqp_empty_bytes,
				amqp_cstring_bytes(_name.c_str()),
				mandatory,
				immediate,
				props ? &origin_props : nullptr,
				bytes),
				"basic publish");
		}
	};

}
