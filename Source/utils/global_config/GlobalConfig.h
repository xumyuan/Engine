#pragma once
#include "utils/Singleton.h"
namespace engine {
	class GlobalConfig : public Singleton<GlobalConfig>
	{
		friend class Singleton<GlobalConfig>;
	public:
		~GlobalConfig();

		std::string getScenePath() { return m_scenePath; }
	private:
		GlobalConfig();
		std::string m_scenePath;
	};
}


