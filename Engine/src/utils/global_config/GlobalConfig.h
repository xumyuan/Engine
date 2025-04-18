#pragma once
#include "utils/Singleton.h"
namespace engine {
	class GlobalConfig : Singleton
	{
	public:
		GlobalConfig();
		~GlobalConfig();
		static GlobalConfig* getInstance();

		std::string getScenePath() { return m_scenePath; }
	private:
		std::string m_scenePath;
	};
}


