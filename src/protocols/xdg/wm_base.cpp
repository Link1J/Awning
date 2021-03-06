#include "wm_base.hpp"
#include "surface.hpp"
#include <spdlog/spdlog.h>
#include "wm/client.hpp"
#include "positioner.hpp"

namespace Awning::Protocols::XDG::WM_Base
{
	const struct xdg_wm_base_interface interface = {
		.destroy           = Interface::Destroy,
		.create_positioner = Interface::Create_Positioner,
		.get_xdg_surface   = Interface::Get_XDG_Surface,
		.pong              = Interface::Pong,
	};

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Create_Positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Positioner::Create(client, 1, id);
		}

		void Get_XDG_Surface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface)
		{
			Surface::Create(client, 1, id, surface);
		}

		void Pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_wm_base_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
		Client::SetWM(wl_client, resource);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &xdg_wm_base_interface, 1, data, Bind);
	}
}