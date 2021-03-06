#pragma once

#include "protocols/handler/xdg-shell.h"
#include <unordered_map>
#include <string>
#include "wm/window.hpp"

namespace Awning::Protocols::XDG::Popup
{
	struct Instance 
	{
		wl_resource* surface;
		wl_resource* parent;
		Window* window;
	};

	extern const struct xdg_popup_interface interface;
	extern std::unordered_map<wl_resource*, Instance> instances;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Grab(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* parent, wl_resource* point);
	void Destroy(struct wl_resource* resource);
}