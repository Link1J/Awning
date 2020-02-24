#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::Protocols::WL::Keyboard
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

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Button(wl_client* client, uint32_t button, bool pressed);
	void ChangeWindow(wl_client* client1, wl_resource* surface1, wl_client* client2, wl_resource* surface2);
}
