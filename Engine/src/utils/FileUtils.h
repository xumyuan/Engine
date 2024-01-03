#pragma once

#include <string>

namespace engine {

	class FileUtils {
	public:
		static std::string readFile(const char* filepath);
	};

}