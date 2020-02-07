#pragma once

#include <wayland-server.h>

#include <unordered_map>

#include "wm/window.hpp"

namespace Awning::Wayland::Pointer
{
	struct Data 
	{
		struct Interface 
		{
			wl_client* client;
			bool inUse = false;
		};

		std::unordered_map<wl_resource*,Interface> pointers;
		WM::Window* window;
		wl_resource* inUse;
	};

	extern const struct wl_pointer_interface interface;
	extern Data data;

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Destroy(struct wl_resource* resource);

	void Enter(wl_client* client, wl_resource* surface, double x, double y);
	void Leave(wl_client* client, wl_resource* surface);
	void Moved(wl_client* client, double x, double y);
	void Button(wl_client* client, uint32_t button, bool pressed);
	void Axis(wl_client* client, uint32_t axis, float value);
	void Frame(wl_client* client);
}
