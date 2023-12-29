#pragma once

#include <string>

namespace engine {

	class FileUtils {
	public:
		static std::string read_file(const char* filepath) {
			// Use the C library to read from a file (faster than fstream)
			FILE* file = fopen(filepath, "rt");
			fseek(file, 0, SEEK_END);
			unsigned long length = ftell(file);
			char* data = new char[length + 1];
			memset(data, 0, length + 1);
			fseek(file, 0, SEEK_SET);
			fread(data, 1, length, file);
			fclose(file);

			std::string result(data);
			delete[] data;
			return result;
		}
	};

}