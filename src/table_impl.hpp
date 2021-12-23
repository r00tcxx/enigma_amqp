#pragma once
#include "amqp.h"
#include "rabbitmq-c/amqp.h"

namespace enigma::amqp {

	class table_impl {
		amqp_table_t _origin_table;
		std::vector<amqp_table_entry_t> _entrys;
	public:
		table_impl() {
			_origin_table = amqp_empty_table;
		}

		table_impl(argument_table& src) {
			make(src);
		}

		void make(argument_table& src) {
			if (src.empty()) {
				_origin_table = amqp_empty_table;
				return;
			}
			_origin_table.num_entries = src.size();
			for (auto& kv : src) {
				amqp_table_entry_t entry;
				entry.key = amqp_cstring_bytes(kv.first.c_str());
				if (std::holds_alternative<std::string>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_UTF8;
					entry.value.value.bytes = amqp_cstring_bytes(std::get<std::string>(kv.second).c_str());
				}
				else if (std::holds_alternative<amqp_type<bool>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_BOOLEAN;
					entry.value.value.boolean = std::get<amqp_type<bool>>(kv.second).value;
				}
				else if (std::holds_alternative<amqp_type<float>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_F32;
					entry.value.value.f32 = std::get<amqp_type<float>>(kv.second).value;
				}
				else if (std::holds_alternative<amqp_type<double>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_F64;
					entry.value.value.f64 = std::get<amqp_type<double>>(kv.second).value;
				}
				else if (std::holds_alternative<amqp_type<std::int8_t>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_I8;
					entry.value.value.i8 = std::get<amqp_type<std::int8_t>>(kv.second).value;
				}
				else if (std::holds_alternative<amqp_type<std::int16_t>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_I16;
					entry.value.value.i16 = std::get<amqp_type<std::int16_t>>(kv.second).value;
				}
				else if (std::holds_alternative<amqp_type<std::int32_t>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_I32;
					entry.value.value.i32 = std::get<amqp_type<std::int32_t>>(kv.second).value;
				}
				else if (std::holds_alternative<amqp_type<std::int64_t>>(kv.second)) {
					entry.value.kind = AMQP_FIELD_KIND_I64;
					entry.value.value.i64 = std::get<amqp_type<std::int64_t>>(kv.second).value;
				}

				_entrys.emplace_back(std::move(entry));
			}
			_origin_table.entries = _entrys.data();
		}

		operator amqp_table_t&(){
			return _origin_table;
		}
	};
}
