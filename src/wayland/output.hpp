#pragma once

#include <wayland-server.h>
#include <unordered_map>
#include <unordered_set>

#include "wm/output.hpp"

namespace Awning::Wayland::Output
{
	struct Data 
	{
		std::unordered_map<WM::Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
		std::unordered_map<wl_resource*, WM::Output::ID> resource_to_outputId;
	};

	extern const struct wl_output_interface interface;
	extern Data data;

	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource* resource);
	}
	
	wl_global* Add    (struct wl_display* display, void* data = nullptr                      );
	void       Bind   (struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
	void       Destroy(struct wl_resource* resource                                          );
}