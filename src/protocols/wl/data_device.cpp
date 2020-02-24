#include "data_device.hpp"
#include "log.hpp"

namespace Awning::Protocols::WL::Data_Device
{
	const struct wl_data_device_interface interface = {
		.start_drag    = Interface::Start_Drag   ,
		.set_selection = Interface::Set_Selection,
		.release       = Interface::Release      ,
	};

	Data data;

	namespace Interface
	{
		void Start_Drag(struct wl_client* client, struct wl_resource* resource, struct wl_resource* source, struct wl_resource* origin, struct wl_resource* icon, uint32_t serial)
		{
			Log::Function::Called("Protocols::WL::Data_Device::Interface");
		}

		void Set_Selection(struct wl_client* client, struct wl_resource* resource, struct wl_resource* source, uint32_t serial)
		{
			Log::Function::Called("Protocols::WL::Data_Device::Interface");
		}

		void Release(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WL::Data_Device::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* seat)
	{
		Log::Function::Called("Protocols::WL::Data_Device");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_data_device_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::WL::Data_Device");

		//if (!data.info.contains(resource))
		//	return;
	}
}
