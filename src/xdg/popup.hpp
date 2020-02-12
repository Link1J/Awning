#pragma once

#include "protocols/xdg-shell-protocol.h"
#include <unordered_map>
#include <string>
#include "wm/window.hpp"

namespace Awning::XDG::Popup
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* surface;
			wl_resource* parent;
			WM::Window* window;
		};

		std::unordered_map<wl_resource*, Instance> popups;
	};

	extern const struct xdg_popup_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Grab(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* parent, wl_resource* point);
	void Destroy(struct wl_resource* resource);
}