#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>

class Thread_Pool
{
private:
	std::mutex m;
	std::vector<std::thread> threads;
	std::queue <std::function<void()>> jobs;
	std::mutex queue_mutex;

	void loadThreads()
	{
		for (int i = 0; i < std::thread::hardware_concurrency(); i++)
		{
			threads.emplace_back(std::thread(&Thread_Pool::ThreadLoop, this));
		}
	}

	void ThreadLoop()
	{
		while (true)
		{
			if (jobs.empty())
				return;
			queue_mutex.lock();
			std::function<void()> job;
			job = jobs.front();
			jobs.pop();
			queue_mutex.unlock();
			job();
		}
	}

	void joinNormal()
	{
		for (int i = 0; threads.size(); i++)
		{
			if (threads[i].joinable())
				threads[i].join();
			//threads[i] = std::thread(&Thread_Pool::ThreadLoop, this);			
		}
		threads.clear();
	}

	void joinEach()
	{
		for (std::thread& active_thread : threads) {
			active_thread.join();
			active_thread = std::thread(&Thread_Pool::ThreadLoop, this);
		}
	}

public:

	Thread_Pool() { loadThreads(); }

	void addJob(const std::function<void()>& job)
	{
		queue_mutex.lock();
		jobs.push(job);
		queue_mutex.unlock();
	}

	void joinThreads()
	{
		joinEach();
	}

	bool busy()
	{
		//queue_mutex.lock();
		//bool poolbusy = true;
		if (jobs.empty())
			return false;
		//queue_mutex.unlock();
		return true;
	}	

	void wait()
	{
		//m.lock();
		while (true)
		{
			if (busy())
				joinThreads();
			else break;
		}
		//m.unlock();
	}

	void stop()
	{
		for (std::thread& active_thread : threads) 
		{
			active_thread.join();
		}
		threads.clear();
	}
};

Thread_Pool threads;