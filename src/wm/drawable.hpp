#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::WM::Drawable
{
	struct Data
	{
		double* xPosition ,* yPosition ;
		double* xDimension,* yDimension;
		char** data;
		bool needsFrame;
		wl_resource* surface;
	};

	extern std::unordered_map<wl_resource*, Awning::WM::Drawable::Data> drawables;
}