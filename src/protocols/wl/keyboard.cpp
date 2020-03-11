#include "keyboard.hpp"
#include "pointer.hpp"
#include "surface.hpp"
#include <spdlog/spdlog.h>

#include "wm/client.hpp"
#include "wm/input.hpp"

#include "utils/shm.hpp"

#include <chrono>
#include <iostream>

uint32_t NextSerialNum();

namespace Awning::Protocols::WL::Keyboard
{
	wl_resource* surface;

	const struct wl_keyboard_interface interface = {
		.release = [](struct wl_client *client, struct wl_resource *resource) 
		{
			Destroy(resource);
		},
	};

	Data data;

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, void* seat)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_keyboard_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		Input::Seat* seat_ = (Input::Seat*)seat;
		
		char* keymap_string = xkb_keymap_get_as_string(seat_->keyboard.keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
		int   keymap_size = strlen(keymap_string) + 1;
		int   keymap_fd = Utils::SHM::AllocateFile(keymap_size);
		void* keymap_ptr = mmap(NULL, keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, keymap_fd, 0);

		wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, keymap_fd, keymap_size);

		memcpy(keymap_ptr, keymap_string, keymap_size);
		munmap(keymap_ptr, keymap_size);
		close(keymap_fd);

		if (version >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
			wl_keyboard_send_repeat_info(resource, 25, 1000);

		data.instances[resource].client  = wl_client;
		data.instances[resource].version = version  ;
		data.instances[resource].seat    = seat     ;

		Client::Bind::Pointer(wl_client, resource);

		Input::Seat::FunctionSet functions = {
			.moved  = nullptr ,
			.button = Button  ,
			.scroll = nullptr ,
			.enter  = Enter   ,
			.leave  = Leave   ,
			.frame  = nullptr ,
			.data   = resource,
		};

		((Input::Seat*)seat)->AddFunctions(0, wl_client, functions);
		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.instances.contains(resource))
			return;
			
		((Input::Seat*)data.instances[resource].seat)->RemoveFunctions(0, data.instances[resource].client, resource);
		Client::Unbind::Keyboard(data.instances[resource].client, resource);
		data.instances.erase(resource);
	}

	void Enter(void* data, void* object, int x, int y)
	{
		if (!data  ) return;
		if (!object) return;

		auto version = Keyboard::data.instances[(wl_resource*)data].version;
		if (version < WL_KEYBOARD_ENTER_SINCE_VERSION) return;

		wl_array* states = new wl_array();
		wl_array_init(states);

		wl_keyboard_send_enter((wl_resource*)data, NextSerialNum(), (wl_resource*)object, states);

		wl_array_release(states);
	}

	void Leave(void* data, void* object)
	{
		if (!data  ) return;
		if (!object) return;

		auto version = Keyboard::data.instances[(wl_resource*)data].version;
		if (version < WL_KEYBOARD_LEAVE_SINCE_VERSION) return;

		wl_keyboard_send_leave((wl_resource*)data, NextSerialNum(), (wl_resource*)object);
	}

	void Button(void* data, uint32_t button, bool state)
	{
		if (!data) return;

		auto version = Keyboard::data.instances[(wl_resource*)data].version;
		if (version < WL_KEYBOARD_KEY_SINCE_VERSION) return;

		uint32_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		Input::Seat* seat = (Input::Seat*)Keyboard::data.instances[(wl_resource*)data].seat;

		xkb_mod_mask_t depressed = xkb_state_serialize_mods  (seat->keyboard.state, XKB_STATE_MODS_DEPRESSED  );
		xkb_mod_mask_t latched   = xkb_state_serialize_mods  (seat->keyboard.state, XKB_STATE_MODS_LATCHED    );
		xkb_mod_mask_t locked    = xkb_state_serialize_mods  (seat->keyboard.state, XKB_STATE_MODS_LOCKED     );
		xkb_mod_mask_t group     = xkb_state_serialize_layout(seat->keyboard.state, XKB_STATE_LAYOUT_EFFECTIVE);

		wl_keyboard_send_key((wl_resource*)data, NextSerialNum(), time, button, state);
		wl_keyboard_send_modifiers((wl_resource*)data, NextSerialNum(), depressed, latched, locked, group);
	}
}