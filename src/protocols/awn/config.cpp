#include "config.hpp"
#include <spdlog/spdlog.h>

#include "config_output.hpp"

namespace Awning::Protocols::AWN::Config
{
	const struct awn_config_interface interface = {
		.destroy  = Interface::Destroy,
		.renderer = Interface::Renderer,
		.output   = Interface::Output,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client *client,struct wl_resource *resource)
		{
		}

		void Renderer(struct wl_client *client, struct wl_resource *resource, uint32_t id)
		{
		}

		void Output(struct wl_client *client, struct wl_resource *resource, uint32_t id)
		{
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &awn_config_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &awn_config_interface, 1, data, Bind);
	}
}