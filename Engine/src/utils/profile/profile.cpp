#include "pch.h"
#include "profile.h"


std::shared_ptr<spdlog::logger> Profile::logger = nullptr;

Profile::Profile(const std::string& name) :name(name), start(std::chrono::high_resolution_clock::now()) {
	initLogger();
	if (logger) {
		logger->info("Profile begin \"{}\"", name);
	}
}

Profile::~Profile() {
	auto duration = std::chrono::high_resolution_clock::now() - start;
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	//std::cout << "Profile \"" << name << "\":" << ms << "ms" << std::endl;
	if (logger) {
		logger->info("Profile end \"{}\": {} ms", name, ms);
	}
}

void Profile::initLogger() {
	if (!logger) {
		// 1. 确保目录存在
		std::filesystem::create_directories("profile_logs");

		// 2. 获取当前时间，生成文件名
		auto now = std::chrono::system_clock::now();
		std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		std::ostringstream oss;
		oss << "profile_logs/profile_"
			<< std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
			<< ".log";
		std::string filename = oss.str();
		// 只初始化一次
		logger = spdlog::basic_logger_mt("profile_logger", filename);
		logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
		logger->set_level(spdlog::level::info);
	}
}


