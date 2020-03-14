#pragma once

#include <wayland-server.h>

namespace Awning::Protocols::WL::Compositor
{
	extern const struct wl_compositor_interface interface;

	namespace Interface
	{
		void Create_Region(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Create_Surface(struct wl_client* client, struct wl_resource* resource, uint32_t id);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}