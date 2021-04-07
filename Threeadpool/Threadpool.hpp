#include "syncqueue.hpp"
#include <mutex>
#include <iostream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <functional>
#pragma once

const int maxTask = 100;

class Threadpool {
public:
	using Task = std::function<void()>;
	Threadpool(int num = std::thread::hardware_concurrency()) :m_queue(maxTask) {
		std::cout << "concurrency :"<<num << std::endl;
		start(num);
	}
	~Threadpool() {
		stop();
	}
	void add(Task&& x) {
		m_queue.put(std::forward<Task>(x));
	}
	void add(const Task& x) {
		m_queue.put(x);
	}
	void stop() {	
		std::call_once(flag, [this] {stopthread(); });
	}

private:
	void start(int num) {
		m_running = true;
		for (int i = 0; i < num; i++) {
			m_pool.push_back(std::make_shared<std::thread>(&Threadpool::runinthread,this));
		}
	}
	
	void runinthread() {
		while (m_running) {
			std::list<Task> l;
			m_queue.take(l);
			for (auto& t : l) {
				if (!m_running) return;
				t();
			}
		}
	}
	void stopthread() {
		m_queue.stop();
		m_running = false;
		for (auto t : m_pool) {
			if(t)t->join();
		}
		m_pool.clear();
	}

private:
	std::list<std::shared_ptr<std::thread>> m_pool;
	std::mutex m_mutex;
	Syncqueue<Task> m_queue;
	std::atomic_bool m_running;
	std::once_flag flag;
};

void test_for_threadpool() {
	Threadpool pool;
	std::thread t1([&pool] {
		for (int i = 0; i < 50; i++) {
			auto id1 = std::this_thread::get_id();
			pool.add([=] {
				std::cout << "thread ID in layer1 :" << id1 << std::endl;
			});
		}
	});

	std::thread t2([&pool] {
		for (int i = 0; i < 50; i++) {
			auto id2 = std::this_thread::get_id();
			pool.add([=] {
				std::cout << "thread ID in layer1 :" << id2 << std::endl;
			});
		}
	});

	std::this_thread::sleep_for(std::chrono::seconds(2));
	getchar();
	pool.stop();
	t1.join();
	t2.join();
}