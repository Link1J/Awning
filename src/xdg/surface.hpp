#pragma once

#include "protocols/xdg-shell-protocol.h"
#include <unordered_map>

namespace Awning::XDG::Surface
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* surface_wl;
			double xPosition, yPosition;
			double xDimension, yDimension;
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

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface);
	void Destroy(struct wl_resource* resource);
}