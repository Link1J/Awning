#pragma once

#include <wayland-server.h>

namespace Awning::Wayland::Region
{
	struct Data 
	{
		struct Instance 
		{
		};
	};

	extern const struct wl_region_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Add(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
		void Subtract(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);
}