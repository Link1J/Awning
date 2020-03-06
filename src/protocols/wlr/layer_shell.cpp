#include "layer_shell.hpp"
#include <spdlog/spdlog.h>
#include "layer_surface.hpp"

namespace Awning::Protocols::WLR::Layer_Shell
{
	const struct zwlr_layer_shell_v1_interface interface = {
		.get_layer_surface = Interface::Get_Layer_Surface
	};
	Data data;

	namespace Interface
	{
		void Get_Layer_Surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, struct wl_resource *output, uint32_t layer, const char *name_space)
		{
			Layer_Surface::Create(client, wl_resource_get_version(resource), id, surface, output, layer);
		}
	}
	
	wl_global* Add (struct wl_display* display, void* data)
	{
		return wl_global_create(display, &zwlr_layer_shell_v1_interface, 1, data, Bind);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_layer_shell_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}
}