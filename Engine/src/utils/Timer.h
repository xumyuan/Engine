#pragma once

namespace engine {

	class Timer {
	private:
		double startTime;
	public:
		Timer();

		void reset();

		inline double elapsed()const { return glfwGetTime() - startTime; }
	};

}