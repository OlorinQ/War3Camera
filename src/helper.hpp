#pragma once

#include "plugin/plugin.hpp"
#include "plugin/unlockfps.hpp"
#include "plugin/showfps.hpp"
#include "plugin/windowfixer.hpp"
#include "plugin/borderless.hpp"
#include "plugin/camera.hpp"
#include "plugin/nowinkey.hpp"
#include <vector>

class Helper
{
	public:
		Helper();
		~Helper();

		void LoadPlugins();
		void UnloadPlugins();

	private:
		std::vector<IPlugin*> m_plugins;
};
