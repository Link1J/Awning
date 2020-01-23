#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::WM::Input
{
	struct Data
	{
		wl_resource* drawable;
		wl_client* client;
	};
}