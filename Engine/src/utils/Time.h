#pragma once

namespace engine {

	struct Time {
	private:
		GLdouble lastFrame;
		GLdouble delta;
	public:
		Time();
		void update();

		inline GLdouble getDeltaTime() const { return delta; }
	};

}