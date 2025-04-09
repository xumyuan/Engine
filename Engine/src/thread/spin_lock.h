#pragma once
#include "pch.h"
#include <thread>

class SpinLock {
public:
	void acquire() { while (m_flag.test_and_set(std::memory_order_acquire)) { std::this_thread::yield(); } }
	void release() { m_flag.clear(std::memory_order_release); }
private:
	std::atomic_flag m_flag;
};

class Guard
{
public:
	Guard(SpinLock& spin_lock) :m_spin_lock(spin_lock) { m_spin_lock.acquire(); }
	~Guard() { m_spin_lock.release(); }

private:
	SpinLock& m_spin_lock;
};
