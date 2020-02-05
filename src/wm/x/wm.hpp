#pragma once
#include <wayland-server.h>

namespace Awning::WM::X
{
	extern wl_client* xWaylandClient;

	void Init();
	void EventLoop();
}