#define _TIMER_
#ifdef _TIMER_

#include <chrono>
#include <thread>

enum time_type {
	nano,
	micro,
	mili,
	sec,
};

class Timer
{
public:
	float time = 0.0f;

	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;

	Timer(){}

	void startClock()
	{
		start = std::chrono::system_clock::now();
	}

	float stopClock(time_type timeMode)
	{
		end = std::chrono::system_clock::now();
		float duration = 0.0f;
		if (timeMode == time_type::micro)
			duration = std::chrono::duration<float, std::micro>(end - start).count();
		else if (timeMode == time_type::mili)
			duration = std::chrono::duration<float, std::milli>(end - start).count();
		else if (timeMode == time_type::sec)
			duration = std::chrono::duration<float>(end - start).count();

		time = duration;

		return duration;
	}
};

#endif // !_TIMER_
