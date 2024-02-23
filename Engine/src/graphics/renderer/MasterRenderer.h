#pragma once

#include "scene/Scene3D.h"

namespace engine
{

	class MasterRenderer
	{
	public:
		MasterRenderer(Scene3D* scene);


	private:
		Scene3D* m_ActiveScene;
	};

}