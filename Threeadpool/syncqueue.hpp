#include <cstdio>
#include <mutex>
#include <iostream>
#include <list>
#include <condition_variable>
#include <thread>
#include <memory>
#pragma once
template<typename T>
class Syncqueue {
public:
	Syncqueue(int size) :maxSize(size), m_stop(false) {}
	
	template<typename F>
	void put(F&& x) {
		Add(std::forward<F>(x));
	}
	template<typename F>
	void put(const F& x) {
		Add(x);
	}

	void take(std::list<T>& l) {

		std::unique_lock<std::mutex> locker(m_mutex);
		notempty_.wait(locker, [this] {return m_stop || notempty(); });
		if (m_stop) { return; }
		l = std::move(m_queue);
		notfull_.notify_one();
	}

	void take(T& x) {
		std::unique_lock<std::mutex> locker(m_mutex);
		notempty_.wait(locker, [this] {return m_stop || notempty(); });
		if (m_stop) { return; }
		x = m_queue.front();
		m_queue.pop_front();
		notfull_.notify_one();
	}

	void stop() {
		{
			std::lock_guard<std::mutex> locker(m_mutex);
			m_stop = true;
		}
		notfull_.notify_all();
		notempty_.notify_all();
	}

	bool empty() {
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.empty();
	}

	bool full() {
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.size() == maxSize;
	}

	size_t size() {
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.size();
	}

private:
	bool notfull()const {
		bool res = m_queue.size() >= maxSize;
		if (res) {
			auto id = std::this_thread::get_id();
			std::cout << "the threadpool is full..., ID: " << id << std::endl;
		}
		return !res;
	}

	bool notempty()const {
		bool res = m_queue.empty();
		if (res) {
			auto id = std::this_thread::get_id();
			std::cout << "the threadpool is empty..., ID: " << id << std::endl;
		}
		return !res;
	}

	template<typename F>
	void Add(F&& x) {
		std::unique_lock<std::mutex> locker(m_mutex);
		notfull_.wait(locker, [this] {return m_stop || notfull(); });
		if (m_stop) { return; }
		m_queue.push_back(std::forward<F>(x));
		notempty_.notify_one();
	}

private:

	std::list<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable notempty_;
	std::condition_variable notfull_;
	int maxSize;
	bool m_stop;

};
