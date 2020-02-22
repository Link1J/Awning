#pragma once

#include "protocols/handler/awn-config.h"

namespace Awning::Protocols::AWN::Config
{
	struct Data 
	{
	};

	extern const struct awn_config_interface interface;
	extern Data data;

	namespace Interface
	{
		void Destroy(struct wl_client *client,struct wl_resource *resource);
		void Renderer(struct wl_client *client, struct wl_resource *resource, uint32_t id);
		void Output(struct wl_client *client, struct wl_resource *resource, uint32_t id);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}