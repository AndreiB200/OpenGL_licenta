#ifndef Thread_Pool_h
#define Thread_Pool_h

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

	std::condition_variable cv;
	std::condition_variable cv_job_added;
	std::condition_variable cv_finished;

	std::atomic<size_t> active_workers{ 0 };
	std::mutex queue_mutex;
	bool should_stop = false;

	void loadThreads()
	{
		for (int i = 0; i < std::thread::hardware_concurrency(); i++)
		{
			threads.emplace_back(std::thread(&Thread_Pool::ThreadLoop, this));
		}
	}

	void ThreadLoop() {
		while (true) {
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				cv_job_added.wait(lock, [this]() {
					return !jobs.empty() || should_stop;
					});

				if (should_stop && jobs.empty()) {
					return;
				}

				job = std::move(jobs.front());
				jobs.pop();

				++active_workers;
			}

			job();
			{
				std::lock_guard<std::mutex> lock(queue_mutex);
				--active_workers;
			}

			cv_finished.notify_one();
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

	Thread_Pool() { loadThreads(); }
public:


	static Thread_Pool& getInstance()
	{
		static Thread_Pool instance;
		return instance;
	}

	void addJob(const std::function<void()>& job)
	{
		{
			std::lock_guard<std::mutex> lock(queue_mutex);
			jobs.push(job);
		}
		cv.notify_one();
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

	void wait() {
		std::unique_lock<std::mutex> lock(queue_mutex);

		cv_finished.wait(lock, [this]() {
			return jobs.empty() && (active_workers == 0);
			});
	}

	void stop()
	{
		for (std::thread& active_thread : threads) 
		{
			active_thread.join();
		}
		threads.clear();
	}

	~Thread_Pool()
	{
		stop();
	}
};

#endif //Thread_Pool_h