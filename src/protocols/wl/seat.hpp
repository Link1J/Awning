#pragma once

#include <wayland-server.h>
#include <unordered_map>

namespace Awning::Protocols::WL::Seat
{
	struct Global 
	{
		struct Instance 
		{
			void* seat;
		};

		std::unordered_map<wl_resource*, Instance> instances;
	};

	extern const struct wl_seat_interface interface;
	extern Global global;
	
	namespace Interface
	{
		void Get_Pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Touch(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Release(struct wl_client* client, struct wl_resource* resource);
	}

	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id);
}