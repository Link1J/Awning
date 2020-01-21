#pragma once

#include <wayland-server.h>
#include <unordered_map>

namespace Awning::Wayland::Surface
{
	struct Data 
	{
		struct Instance 
		{
			double xDimension, yDimension;
			bool damaged = false;
			char* data = nullptr;
			wl_resource* buffer;
		};

		std::unordered_map<wl_resource*, Instance> surfaces;
	};

	extern const struct wl_surface_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Attach(struct wl_client* client, struct wl_resource* resource, struct wl_resource* buffer, int32_t x, int32_t y);
		void Damage(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
		void Frame(struct wl_client* client, struct wl_resource* resource, uint32_t callback);
		void Set_Opaque_Region(struct wl_client* client, struct wl_resource* resource, struct wl_resource* region);
		void Set_Input_Region(struct wl_client* client, struct wl_resource* resource, struct wl_resource* region);
		void Commit(struct wl_client* client, struct wl_resource* resource);
		void Set_Buffer_Transform(struct wl_client* client, struct wl_resource* resource, int32_t transform);
		void Set_Buffer_Scale(struct wl_client* client, struct wl_resource* resource, int32_t scale);
		void Damage_Buffer(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);
}