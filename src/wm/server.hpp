#pragma once
#include <wayland-server-core.h>
#include <string>

namespace Awning::Server
{
	extern wl_display        * display;
	extern wl_event_loop     * event_loop;
	extern wl_protocol_logger* logger; 
	extern wl_listener         client_listener;
	extern std::string         socketname;

	void Init();
};
