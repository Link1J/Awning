#pragma once

#include "protocols/xdg-shell-protocol.h"

namespace Awning::XDG::WM_Base
{
	struct Data 
	{
	};

	extern const struct xdg_wm_base_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Create_Positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_XDG_Surface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface);
		void Pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial);
	}

	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}