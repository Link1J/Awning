#pragma once

#include "protocols/handler/xdg-shell.h"
#include <unordered_map>

#include "wm/window.hpp"

namespace Awning::Protocols::XDG::Surface
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* surface_wl;
			wl_resource* toplevel;
			wl_resource* popup;
			WM::Window* window;
			bool configured = false;
		};

		std::unordered_map<wl_resource*, Instance> surfaces;
	};

	extern const struct xdg_surface_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Get_Toplevel(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Popup(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* parent, struct wl_resource* positioner);
		void Set_Window_Geometry(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
		void Ack_Configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface);
	void Destroy(struct wl_resource* resource);
}