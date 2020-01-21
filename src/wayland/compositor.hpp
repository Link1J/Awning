#pragma once

#include <wayland-server.h>

namespace Awning::Wayland::Compositor
{
	struct Data 
	{
		wl_global* global;
	};

	extern const struct wl_compositor_interface interface;
	extern Data data;

	namespace Interface
	{
		void Create_Region(struct wl_client *client, struct wl_resource *resource, uint32_t id);
		void Create_Surface(struct wl_client *client, struct wl_resource *resource, uint32_t id);
	}
	
	void Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}