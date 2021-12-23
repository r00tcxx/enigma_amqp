#pragma once
#include <sstream>
#include "rabbitmq-c/amqp.h"
#include "rabbitmq-c/tcp_socket.h"

#define throw_if(cond, e) \
do { if(cond) throw std::runtime_error(e); } while (0);

namespace enigma::amqp {

	struct amqp_reply {
		static void check(
            amqp_rpc_reply_t income,
            const std::string& context) {
            std::stringstream ss;
            switch (income.reply_type) {
            case AMQP_RESPONSE_NORMAL:
                return;

            case AMQP_RESPONSE_NONE:
                ss << "amqp [" << context << "]: " << "missing RPC reply type";
                throw std::runtime_error(ss.str());

            case AMQP_RESPONSE_LIBRARY_EXCEPTION:
                ss << "amqp [" << context << "]: " << amqp_error_string2(income.library_error);
                throw std::runtime_error(ss.str());

            case AMQP_RESPONSE_SERVER_EXCEPTION:
                switch (income.reply.id) {
                case AMQP_CONNECTION_CLOSE_METHOD: {
                    amqp_connection_close_t* m = (amqp_connection_close_t*)income.reply.decoded;
                    ss << "amqp [" << context << "]: " 
                        << "server connection error - "
                        << (char*)m->reply_text.bytes;

                    throw std::runtime_error(ss.str());
                }
                case AMQP_CHANNEL_CLOSE_METHOD: {
                    amqp_channel_close_t* m = (amqp_channel_close_t*)income.reply.decoded;
                    ss << "amqp [" << context << "]: "
                        << "code = "
                        << m->reply_code << " - "
                        << (char*)m->reply_text.bytes;
                    throw std::runtime_error(ss.str());
                }
                default:
                    ss << "amqp [" << context << "]: "
                        << "unknown server error, method id = "
                        << income.reply.id;
                    throw std::runtime_error(ss.str());
                }
            }
		}

        static void check(int code,
            const std::string& context,
            const std::string& detail = "") {
            std::stringstream ss;
            ss << "amqp [" << context << "]: " << "code = " << code << detail;
            throw_if(code < 0, ss.str());
        }

	};

}
