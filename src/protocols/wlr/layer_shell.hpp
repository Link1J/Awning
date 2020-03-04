#pragma once
#include "protocols/handler/wlr-layer-shell.h"

namespace Awning::Protocols::WLR::Layer_Shell
{
	struct Data 
	{
	};

	extern const struct zwlr_layer_shell_v1_interface interface;
	extern Data data;

	namespace Interface
	{
		void Get_Layer_Surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, struct wl_resource *output, uint32_t layer, const char *name_space);
	}
	
	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}