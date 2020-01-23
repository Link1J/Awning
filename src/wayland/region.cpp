#include "region.hpp"
#include "log.hpp"

namespace Awning::Wayland::Region
{
	const struct wl_region_interface interface = {
		.destroy = Interface::Destroy,
		.add = Interface::Add,
		.subtract = Interface::Subtract
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Wayland::Region::Interface");
		}

		void Add(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			Log::Function::Called("Wayland::Region::Interface");
		}
		
		void Subtract(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			Log::Function::Called("Wayland::Region::Interface");
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Wayland::Region");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_region_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Wayland::Region");
	}
}