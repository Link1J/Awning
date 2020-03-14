#pragma once

#include <wayland-server.h>
#include <unordered_map>
#include <unordered_set>

#include "wm/output.hpp"

namespace Awning::Protocols::WL::Output
{
	extern const struct wl_output_interface interface;
	extern std::unordered_map<Awning::Output::ID, std::unordered_set<wl_resource*>> outputId_to_resource;
	extern std::unordered_map<wl_resource*, Awning::Output::ID> resource_to_outputId;
	
	namespace Interface
	{
		void Release(struct wl_client *client, struct wl_resource* resource);
	}
	
	wl_global* Add    (struct wl_display* display, void* data = nullptr                      );
	void       Bind   (struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
	void       Destroy(struct wl_resource* resource                                          );
}