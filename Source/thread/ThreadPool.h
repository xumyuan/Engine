#pragma once

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class TaskGroup;

class ThreadPool
{
public:
#if defined(__cpp_lib_move_only_function)
	using Job = std::move_only_function<void()>;
#else
	using Job = std::function<void()>;
#endif

	explicit ThreadPool(size_t thread_count = 0);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	size_t threadCount() const { return m_threads.size(); }
	bool isWorkerThread() const;

	template<class F>
	auto submit(F&& f) -> std::future<std::invoke_result_t<std::decay_t<F>&>>;

	// body(begin, end) 处理 [begin, end)，返回前所有区间都已执行完；任一区间抛出的异常在这里重新抛出
	template<class F>
	void parallelFor(size_t count, F&& body, size_t grain = 1);

private:
	friend class TaskGroup;

	void enqueue(Job job);
	void enqueueBatch(std::vector<Job>& jobs);
	void workerLoop();

	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::deque<Job> m_jobs;
	bool m_stopping = false;
	std::vector<std::thread> m_threads;
};

// 一批任务的完成计数；wait() 只等本组提交的任务，并重新抛出组内第一个异常
// 不能在同一线程池的 worker 线程里 wait()，否则 worker 会等待自己
class TaskGroup
{
public:
	explicit TaskGroup(ThreadPool& pool) : m_pool(pool) {}
	~TaskGroup() { waitIdle(); }

	TaskGroup(const TaskGroup&) = delete;
	TaskGroup& operator=(const TaskGroup&) = delete;

	template<class F>
	void run(F&& f) {
		addPending(1);
		m_pool.enqueue(wrap(std::forward<F>(f)));
	}

	void wait() {
		if (std::exception_ptr error = waitIdle()) {
			std::rethrow_exception(error);
		}
	}

private:
	friend class ThreadPool;

	template<class F>
	ThreadPool::Job wrap(F&& f) {
		return [this, fn = std::forward<F>(f)]() mutable {
			try {
				fn();
			}
			catch (...) {
				recordError(std::current_exception());
			}
			finishOne();
		};
	}

	void addPending(size_t n) {
		std::lock_guard lock(m_mutex);
		m_pending += n;
	}

	void recordError(std::exception_ptr error) {
		std::lock_guard lock(m_mutex);
		if (!m_firstError) {
			m_firstError = std::move(error);
		}
	}

	void finishOne() {
		std::lock_guard lock(m_mutex);
		// 持锁通知：等待方一返回就可能析构本对象
		if (--m_pending == 0) {
			m_cv.notify_all();
		}
	}

	std::exception_ptr waitIdle() {
		std::unique_lock lock(m_mutex);
		assert((m_pending == 0 || !m_pool.isWorkerThread()) && "TaskGroup::wait() on a worker of the same pool deadlocks");
		m_cv.wait(lock, [&] { return m_pending == 0; });
		return std::exchange(m_firstError, nullptr);
	}

	ThreadPool& m_pool;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	size_t m_pending = 0;
	std::exception_ptr m_firstError;
};

template<class F>
auto ThreadPool::submit(F&& f) -> std::future<std::invoke_result_t<std::decay_t<F>&>> {
	using R = std::invoke_result_t<std::decay_t<F>&>;
	// Job 可能退回到要求可拷贝的 std::function，packaged_task 只能移动，所以放进 shared_ptr
	auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
	std::future<R> future = task->get_future();
	enqueue([task] { (*task)(); });
	return future;
}

template<class F>
void ThreadPool::parallelFor(size_t count, F&& body, size_t grain) {
	if (count == 0) {
		return;
	}

	const size_t target_chunks = threadCount() * 4;
	const size_t chunk = std::max<size_t>(std::max<size_t>(grain, 1), (count + target_chunks - 1) / target_chunks);
	const size_t chunk_count = (count + chunk - 1) / chunk;

	if (chunk_count == 1 || isWorkerThread()) {
		body(size_t{ 0 }, count);
		return;
	}

	TaskGroup group(*this);
	std::vector<Job> jobs;
	jobs.reserve(chunk_count - 1);
	for (size_t begin = chunk; begin < count; begin += chunk) {
		const size_t end = std::min(begin + chunk, count);
		jobs.push_back(group.wrap([&body, begin, end] { body(begin, end); }));
	}
	group.addPending(jobs.size());
	enqueueBatch(jobs);

	std::exception_ptr caller_error;
	try {
		body(size_t{ 0 }, chunk);
	}
	catch (...) {
		caller_error = std::current_exception();
	}

	std::exception_ptr worker_error = group.waitIdle();
	if (caller_error) {
		std::rethrow_exception(caller_error);
	}
	if (worker_error) {
		std::rethrow_exception(worker_error);
	}
}

// 首次调用时创建；函数内静态对象保证先于依赖它的静态 TaskGroup 构造、后于它们析构
ThreadPool& globalThreadPool();
