#include "subcompositor.hpp"
#include "subsurface.hpp"
#include "region.hpp"
#include <spdlog/spdlog.h>

namespace Awning::Protocols::WL::Subcompositor
{
	const struct wl_subcompositor_interface interface = {
		.get_subsurface = Interface::Get_Subsurface
	};

	namespace Interface
	{
		void Get_Subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface, struct wl_resource* parent)
		{
			Subsurface::Create(client, wl_resource_get_version(resource), id, surface, parent);
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_subcompositor_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_subcompositor_interface, 1, data, Bind);
	}
}