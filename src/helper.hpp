#pragma once

#include "plugin/plugin.hpp"
#include "plugin/camera.hpp"
#include <vector>

class Helper
{
	public:
		Helper();
		~Helper();

		void LoadPlugins();
		void UnloadPlugins(bool exiting);

	private:
		std::vector<IPlugin*> m_plugins;
};
