#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::Protocols::WL::Keyboard
{
	struct Instance 
	{
		wl_client* client;
		int version = 0;
		void* seat;
	};

	extern const struct wl_keyboard_interface interface;
	extern std::unordered_map<wl_resource*, Instance> instances;

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, void* seat);
	void Destroy(struct wl_resource* resource);

	void Enter (void* data, void* object, int x, int y );
	void Leave (void* data, void* object               );
	void Button(void* data, uint32_t button, bool state);
}
