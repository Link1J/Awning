#pragma once
#include <wayland-server-core.h>
#include <string>

namespace Awning::Server
{
	struct Global
	{
		wl_display        * display;
		wl_event_loop     * event_loop;
		wl_protocol_logger* logger; 
		wl_listener         client_listener;
		std::string         socketname;
	};
	extern Global global;

	void Init();
};
