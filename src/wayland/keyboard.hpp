#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::Wayland::Keyboard
{
	struct Data 
	{
		struct Interface 
		{
			wl_resource* resource;
		};

		std::unordered_map<wl_client*,Interface> keyboards;
		wl_resource* pre_shell;
		bool moveMode = false;
	};

	extern const struct wl_keyboard_interface interface;
	extern Data data;

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Button(uint32_t button, bool pressed);
}