#pragma once
#include "pch.h"


#include "spin_lock.h"

class Task
{
public:
	virtual void run() = 0;
	virtual ~Task() = default;
};

struct TextureLoadTask :public Task {
public:
	TextureLoadTask(std::function<void(void)> lambda) : m_lambda(lambda) {}

	void run() override {
		m_lambda();
	}
private:
	//std::string m_path;
	std::function<void(void)> m_lambda;
};


class ThreadPool
{
public:

	static void WorkerThread(ThreadPool* master);
	ThreadPool(size_t thread_count = 0);
	~ThreadPool();

	void parallelFor(size_t width, size_t height, const std::function<void(size_t, size_t)>& lambda, bool complex = true);
	void wait()const;

	void addTask(Task* task);
	Task* getTask();
private:
	std::atomic<int> m_alive;
	std::atomic<int> pending_task_count;
	std::vector<std::thread> m_threads;
	std::queue<Task*> m_tasks;
	SpinLock m_spin_lock;
};

extern ThreadPool thread_pool;