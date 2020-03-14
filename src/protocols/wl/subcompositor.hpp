#pragma once

#include <wayland-server.h>

namespace Awning::Protocols::WL::Subcompositor
{
	extern const struct wl_subcompositor_interface interface;

	namespace Interface
	{
		void Get_Subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface, struct wl_resource* parent);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}