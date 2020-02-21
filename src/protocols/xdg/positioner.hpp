#pragma once

#include "protocols/handler/xdg-shell-protocol.h"
#include <unordered_map>
#include <string>

namespace Awning::Protocols::XDG::Positioner
{
	struct Data 
	{
		struct Instance 
		{
			int32_t width, height;
			int32_t x, y;
		};

		std::unordered_map<wl_resource*, Instance> instances;
	};

	extern const struct xdg_positioner_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height);
		void Set_Anchor_Rect(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
		void Set_Anchor(struct wl_client* client, struct wl_resource* resource, uint32_t anchor);
		void Set_Gravity(struct wl_client* client, struct wl_resource* resource, uint32_t gravity);
		void Set_Constraint_Adjustment(struct wl_client* client, struct wl_resource* resource, uint32_t constraint_adjustment);
		void Set_Offset(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);
}