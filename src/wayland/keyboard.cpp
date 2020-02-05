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

		data.keyboards[wl_client].resource = resource;

		WM::Client::Bind::Keyboard(wl_client, resource);
	}

	void Button(wl_client* client, uint32_t button, bool released)
	{
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		for (auto resource : WM::Client::Get::All::Keyboards(client))
			wl_keyboard_send_key((wl_resource*)resource, NextSerialNum(), time, button, released);
	}
}