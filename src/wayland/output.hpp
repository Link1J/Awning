#pragma once

#include <wayland-server.h>

namespace Awning::Wayland::Output
{
	struct Data 
	{
	};

	extern const struct wl_output_interface interface;
	extern Data data;

	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource *resource);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}