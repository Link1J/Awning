#pragma once

#include <wayland-server.h>

namespace Awning::Protocols::WL::Shell
{
	struct Data 
	{
	};

	extern const struct wl_shell_interface interface;
	extern Data data;

	namespace Interface
	{
		void Get_Shell_Surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}