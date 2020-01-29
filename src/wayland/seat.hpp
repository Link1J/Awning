#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::Wayland::Seat
{
	struct Data 
	{
		wl_global* global;
	};

	extern const struct wl_seat_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Get_Pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Touch(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Release(struct wl_client* client, struct wl_resource* resource);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}