#pragma once

#include <wayland-server.h>

namespace Awning::Wayland::Output
{
	struct Data 
	{
		wl_global* global;
	};

	extern const struct wl_output_interface interface;
	extern Data data;

	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource *resource);
	}
	
	void Add (struct wl_display* display                                            );
	void Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}