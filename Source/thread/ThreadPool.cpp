#include "pch.h"
#include "ThreadPool.h"

namespace {
	thread_local const ThreadPool* t_owner_pool = nullptr;
}

ThreadPool::ThreadPool(size_t thread_count /* = 0 */) {
	if (thread_count == 0) {
		thread_count = std::max(1u, std::thread::hardware_concurrency());
	}
	m_threads.reserve(thread_count);
	for (size_t i = 0; i < thread_count; i++) {
		m_threads.emplace_back([this] { workerLoop(); });
	}
}

ThreadPool::~ThreadPool() {
	{
		std::lock_guard lock(m_mutex);
		m_stopping = true;
	}
	m_cv.notify_all();
	for (auto& thread : m_threads) {
		thread.join();
	}
}

bool ThreadPool::isWorkerThread() const {
	return t_owner_pool == this;
}

void ThreadPool::enqueue(Job job) {
	{
		std::lock_guard lock(m_mutex);
		assert(!m_stopping);
		m_jobs.push_back(std::move(job));
	}
	m_cv.notify_one();
}

void ThreadPool::enqueueBatch(std::vector<Job>& jobs) {
	if (jobs.empty()) {
		return;
	}
	{
		std::lock_guard lock(m_mutex);
		assert(!m_stopping);
		const size_t old_size = m_jobs.size();
		try {
			for (auto& job : jobs) {
				m_jobs.push_back(std::move(job));
			}
		}
		catch (...) {
			m_jobs.resize(old_size);
			throw;
		}
	}
	jobs.clear();
	m_cv.notify_all();
}

void ThreadPool::workerLoop() {
	t_owner_pool = this;
	for (;;) {
		Job job;
		{
			std::unique_lock lock(m_mutex);
			m_cv.wait(lock, [&] { return m_stopping || !m_jobs.empty(); });
			if (m_jobs.empty()) {
				return;
			}
			job = std::move(m_jobs.front());
			m_jobs.pop_front();
		}
		job();
	}
}

ThreadPool& globalThreadPool() {
	static ThreadPool pool;
	return pool;
}
