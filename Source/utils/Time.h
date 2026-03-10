#pragma once

namespace engine {

	struct Time {
	private:
		double lastFrame;
		double delta;
	public:
		Time();
		void update();

		inline double getDeltaTime() const { return delta; }
	};

}