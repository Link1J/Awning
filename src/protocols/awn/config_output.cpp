#include "config_output.hpp"
#include "log.hpp"

namespace Awning::Protocols::AWN::Config_Output
{
	const struct awn_config_output_interface interface = {
		.destroy      = Interface::Destroy     ,
		.set_mode     = Interface::Set_Mode    ,
		.set_position = Interface::Set_Position,
	};

	Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::AWN::Config::Interface");
		}

		void Set_Mode(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output, int32_t mode)
		{
			Log::Function::Called("Protocols::AWN::Config::Interface");
		}

		void Set_Position(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output, int32_t x, int32_t y)
		{
			Log::Function::Called("Protocols::AWN::Config::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Protocols::AWN::Config");

		struct wl_resource* resource = wl_resource_create(wl_client, &awn_config_output_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::AWN::Config");
	}
}