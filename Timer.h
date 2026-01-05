#pragma once

//#include <iostream>
#include <chrono>
#include <thread>

enum type {
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

	void startClock()
	{
		start = std::chrono::system_clock::now();
	}

	float stopClock(type timeMode)
	{
		end = std::chrono::system_clock::now();
		float duration = 0.0f;
		if (timeMode == micro)
			duration = std::chrono::duration<float, std::micro>(end - start).count();
		else if (timeMode == mili)
			duration = std::chrono::duration<float, std::milli>(end - start).count();
		else if (timeMode == sec)
			duration = std::chrono::duration<float>(end - start).count();

		time = duration;

		return duration;
	}
};