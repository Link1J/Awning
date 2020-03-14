#include "data_device_manager.hpp"
#include <spdlog/spdlog.h>

#include "backends/manager.hpp"

#include "data_source.hpp"
#include "data_device.hpp"

namespace Awning::Protocols::WL::Data_Device_Manager
{
	const struct wl_data_device_manager_interface interface = {
		.create_data_source = Interface::Create_Data_Source,
        .get_data_device    = Interface::Get_Data_Device,
	};

	namespace Interface
	{
		void Create_Data_Source(struct wl_client* client, struct wl_resource* resource, uint32_t id)
        {
			Data_Source::Create(client, wl_resource_get_version(resource), id);
        }

		void Get_Data_Device(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat)
        {
			Data_Device::Create(client, wl_resource_get_version(resource), id, seat);
        }
    }

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{		
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_data_device_manager_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_data_device_manager_interface, 1, data, Bind);
	}
}