#pragma once

#include <wayland-server.h>
#include <unordered_map>

#include "wm/window.hpp"

namespace Awning::Protocols::WL::Subsurface
{
	struct Instance 
	{
		Window* window;
		wl_resource* surface;
	};

	extern const struct wl_subsurface_interface interface;
	extern std::unordered_map<wl_resource*, Instance> instances;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Position(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y);
		void Place_Above(struct wl_client* client, struct wl_resource* resource, struct wl_resource* sibling);
		void Place_Below(struct wl_client* client, struct wl_resource* resource, struct wl_resource* sibling);
		void Set_Sync(struct wl_client* client, struct wl_resource* resource);
		void Set_Desync(struct wl_client* client, struct wl_resource* resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* surface, struct wl_resource* parent);
	void Destroy(struct wl_resource* resource);
}

