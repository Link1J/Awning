#include "compositor.hpp"
#include "surface.hpp"
#include "region.hpp"
#include "log.hpp"

namespace Awning::Protocols::WL::Compositor
{
	const struct wl_compositor_interface interface = {
		.create_surface = Interface::Create_Surface,
		.create_region = Interface::Create_Region
	};

	Data data;

	namespace Interface
	{
		void Create_Region(struct wl_client* client, struct wl_resource* resource, uint32_t id) 
		{
			Log::Function::Called("Protocols::WL::Compositor::Interface");
			Region::Create(client, wl_resource_get_version(resource), id);
		}

		void Create_Surface(struct wl_client* client, struct wl_resource* resource, uint32_t id) 
		{
			Log::Function::Called("Protocols::WL::Compositor::Interface");
			Surface::Create(client, wl_resource_get_version(resource), id);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Protocols::WL::Compositor");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_compositor_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_compositor_interface, 4, data, Bind);
	}
}