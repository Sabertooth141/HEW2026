#pragma once
#include <chrono>

class Timer
{
public:
	Timer()
	{
		last = std::chrono::steady_clock::now();
	}

	float Mark()
	{
		const auto now = std::chrono::steady_clock::now();
		const std::chrono::duration<float> deltaTime = now - last;
		last = now;
		return deltaTime.count();
	}

	float Peek() const
	{
		const std::chrono::duration<float> deltaTime = std::chrono::steady_clock::now() - last;
		return deltaTime.count();
	}

private:
	std::chrono::steady_clock::time_point last;
};
