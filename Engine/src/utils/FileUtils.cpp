#include "pch.h"
#include "FileUtils.h"

namespace engine {
	std::string FileUtils::readFile(const std::string& filepath) {
		std::ifstream ifs(filepath, std::ios::in, std::ios::binary);
		std::string result;

		if (ifs)
		{
			result = std::string((std::istreambuf_iterator<char>(ifs)),
				(std::istreambuf_iterator<char>()));
			ifs.close();
		}
		else
		{
			std::cout << "File read error:" << filepath << std::endl;
		}

		return result;
		return result;
	}
}