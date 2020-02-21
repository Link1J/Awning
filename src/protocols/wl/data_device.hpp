#pragma once

#include <wayland-server.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace Awning::Protocols::WL::Data_Device
{
	struct Data 
	{
	};

	extern const struct wl_data_device_interface interface;
	extern Data data;

	namespace Interface
	{
		void Start_Drag(struct wl_client* client, struct wl_resource* resource, struct wl_resource* source, struct wl_resource* origin, struct wl_resource* icon, uint32_t serial);
		void Set_Selection(struct wl_client* client, struct wl_resource* resource, struct wl_resource* source, uint32_t serial);
		void Release(struct wl_client* client, struct wl_resource* resource);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* seat);
	void Destroy(struct wl_resource* resource);
}
