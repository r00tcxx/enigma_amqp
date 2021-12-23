// test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <mutex>
#include <random>
#include <condition_variable>
#include "amqp.h"
using namespace enigma::amqp;

std::mutex m;
std::condition_variable cond;

void test_exchange() {
	connection conn;
	try {
		conn.create(
			"/",
			"guest",
			"guest",
			1000,
			"127.0.0.1");
		auto chan = conn.get_channel();
		auto exchange = chan->declare_exchange(
			"test_exchange2",
			"fanout",
			false,
			true,
			false,
			false);

		std::unique_lock<std::mutex> l(m);
		cond.wait(l);

		for (int i = 0; i < 500; ++i) {
			auto body = std::to_string(i);
			exchange->publish("r", false, false, body.c_str(), body.length());
		//	std::this_thread::sleep_for(std::chrono::milliseconds());
		}
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void test_delay_exchange() {
	connection conn;
	try {
		conn.create(
			"/",
			"guest",
			"guest",
			1000,
			"127.0.0.1");
		auto chan = conn.get_channel();

		argument_table args;
		args["x-delayed-type"] = table_value("direct");

		auto exchange = chan->declare_exchange(
			"delayed_exchange",
			"x-delayed-message",
			false,
			true,
			false,
			false,
			std::move(args));

		std::unique_lock<std::mutex> l(m);
		cond.wait(l);

		std::default_random_engine e;
		std::uniform_int_distribution<std::int32_t>u(100, 8000);
		for (int i = 0; i < 500; ++i) {
			auto body = std::to_string(i);
			properties props;
			argument_table headers;
			auto random_delay = u(e);
			std::cout << "public delay message, " << random_delay << std::endl;
			headers["x-delay"] = amqp_type{ random_delay };
			props.headers = headers;
			exchange->publish("", false, false, body.c_str(), body.length(), props);

		//	std::this_thread::sleep_for(std::chrono::milliseconds());
		}
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void test_delay_queue() {
	connection conn;
	try {
		conn.create(
			"/",
			"guest",
			"guest",
			1000,
			"127.0.0.1");
		auto chan = conn.get_channel();
		//argument_table args;
		//args["x-message-ttl"] = table_value(amqp_type{ 5000 });
		//args["x-dead-letter-exchange"] = table_value("dead_test");
		//args["x-dead-letter-routing-key"] = table_value("r");

		auto que = chan->declare_queue("delayed_queue",
			false,
			false,
			false,
			false);

		que->bind(
			"delayed_exchange",
			"");
		cond.notify_all();

		int i = 1;
		que->using_fair_consume();
		for (;; ++i) {
			que->consume(false, [&](message* msg) {
				std::cout << "recv: " << msg->get_body() << std::endl;
				msg->ack(true);
			});
		}
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void test_queue() {
	connection conn;
	try {
		conn.create(
			"/",
			"guest",
			"guest",
			1000,
			"127.0.0.1");
		auto chan = conn.get_channel();
		argument_table args;
		args["x-message-ttl"] = table_value(amqp_type{ 5000 });
		args["x-dead-letter-exchange"] = table_value("dead_test");
		args["x-dead-letter-routing-key"] = table_value("r");

		auto que = chan->declare_queue("test_queue",
			false,
			false,
			false,
			false,
			std::move(args));

		que->bind(
			"test_exchange",
			"r");
		cond.notify_all();

		int i = 1;
		que->using_fair_consume();
		for (;;++i) {
			que->consume(false, [&](message* msg) {
				if (i % 2) {
					std::cout << "recv: " << msg->get_body() << std::endl;
					msg->ack(true);
				}
				else {
					std::cout << "reject: " << msg->get_body() << std::endl;
					msg->reject(true);
					msg->requeue(false);
				}
			});
		}
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

int main() {
	std::thread th1(test_delay_queue);
	std::thread th2(test_delay_exchange);

	th1.join();
	th2.join();
	return 0;

}

