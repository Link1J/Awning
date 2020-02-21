#include "data_device_manager.hpp"
#include "log.hpp"

#include "backends/manager.hpp"

namespace Awning::Protocols::WL::Data_Device_Manager
{
	const struct wl_data_device_manager_interface interface = {
		.create_data_source = Interface::Create_Data_Source,
        .get_data_device    = Interface::Get_Data_Device,
	};

	Data data;

	namespace Interface
	{
		void Create_Data_Source(struct wl_client *client, struct wl_resource *resource, uint32_t id)
        {
		    Log::Function::Called("Wayland::Data_Device_Manager::Interface");
        }

		void Get_Data_Device(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *seat)
        {
		    Log::Function::Called("Wayland::Data_Device_Manager::Interface");
        }
    }

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Wayland::Data_Device_Manager");
		
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_data_device_manager_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_data_device_manager_interface, 3, data, Bind);
	}
}