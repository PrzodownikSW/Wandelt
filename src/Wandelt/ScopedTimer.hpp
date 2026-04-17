#pragma once

#include <chrono>

#include "Wandelt/Defines.hpp"

namespace Wandelt
{

	class ScopedTimer
	{
	public:
		// Automatically starts the timer when an instance is created
		ScopedTimer() : m_StartTime(std::chrono::high_resolution_clock::now()) {}

		void Reset() { m_StartTime = std::chrono::high_resolution_clock::now(); }

		f64 GetElapsedSeconds() const
		{
			std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();

			return std::chrono::duration_cast<std::chrono::duration<f64>>(end_time - m_StartTime).count();
		}

		f64 GetElapsedMilliseconds() const { return GetElapsedSeconds() * 1000.0; }

	private:
		std::chrono::high_resolution_clock::time_point m_StartTime;
	};

} // namespace Wandelt
