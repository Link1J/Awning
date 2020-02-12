#include "keyboard.hpp"
#include "pointer.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wm/client.hpp"

#include <chrono>
#include <iostream>

uint32_t NextSerialNum();

namespace Awning::Wayland::Keyboard
{
	const struct wl_keyboard_interface interface = {
		.release = [](struct wl_client *client, struct wl_resource *resource) 
		{
			Log::Function::Called("Wayland::Keyboard::interface.release");
		},
	};

	Data data;

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Wayland::Keyboard");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_keyboard_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, nullptr);
		wl_keyboard_send_keymap(resource, 0, 0, 0);
		wl_keyboard_send_repeat_info(resource, 0, 0);

		data.keyboards[wl_client].resource = resource;

		WM::Client::Bind::Keyboard(wl_client, resource);
	}

	void Button(wl_client* client, uint32_t button, bool released)
	{
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		for (auto resource : WM::Client::Get::All::Keyboards(client))
		{
			wl_keyboard_send_key((wl_resource*)resource, NextSerialNum(), time, button, released);
		}
	}

	void ChangeWindow(wl_client* client1, wl_resource* surface1, wl_client* client2, wl_resource* surface2)
	{
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		for (auto resource : WM::Client::Get::All::Keyboards(client1))
		{
			wl_keyboard_send_leave((wl_resource*)resource, NextSerialNum(), surface1);
		}

		for (auto resource : WM::Client::Get::All::Keyboards(client2))
		{
			wl_array* states = new wl_array();
			wl_array_init(states);

			wl_keyboard_send_enter((wl_resource*)resource, NextSerialNum(), surface2, states);

			wl_array_release(states);
			delete states;
		}
	}
}