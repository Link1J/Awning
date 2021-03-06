#pragma once

#include <wayland-server.h>

namespace Awning::Protocols::WL::Data_Device_Manager
{
	extern const struct wl_data_device_manager_interface interface;

	namespace Interface
	{
		void Create_Data_Source(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Data_Device(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat);
    }
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}