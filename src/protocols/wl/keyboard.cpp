#include "keyboard.hpp"
#include "pointer.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wm/client.hpp"

#include <xkbcommon/xkbcommon.h>

#include <chrono>
#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

uint32_t NextSerialNum();

static void randname(char *buf) 
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A'+(r&15)+(r&16)*2;
		r >>= 5;
	}
}

int create_shm_file(void) {
	int retries = 100;
	do {
		char name[] = "/Awning-XXXXXX";
		randname(name + strlen(name) - 6);

		--retries;
		// CLOEXEC is guaranteed to be set by shm_open
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			shm_unlink(name);
			return fd;
		}
	} while (retries > 0 && errno == EEXIST);

	return -1;
}

int allocate_shm_file(size_t size) 
{
	int fd = create_shm_file();
	if (fd < 0) {
		return -1;
	}

	int ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

namespace Awning::Protocols::WL::Keyboard
{
	struct xkb_context* ctx;
	struct xkb_keymap* keymap;
	char* keymap_string;
	int keymap_size;
	int keymap_fd;
	void* keymap_ptr;
	struct xkb_state* state;

	wl_resource* surface;

	const struct wl_keyboard_interface interface = {
		.release = [](struct wl_client *client, struct wl_resource *resource) 
		{
			Log::Function::Called("Protocols::WL::Keyboard::interface.release");
		},
	};

	Data data;

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Protocols::WL::Keyboard");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_keyboard_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}

		if (!ctx)
		{
			struct xkb_rule_names names = {
			    .rules   = NULL,
			    .model   = NULL,
			    .layout  = NULL,
			    .variant = NULL,
			    .options = NULL
			};
			
			ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
			keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
			keymap_string = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
			keymap_size = strlen(keymap_string) + 1;
			keymap_fd = allocate_shm_file(keymap_size);
			keymap_ptr = mmap(NULL, keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, keymap_fd, 0);
			state = xkb_state_new(keymap);

			memcpy(keymap_ptr, keymap_string, keymap_size);
			munmap(keymap_ptr, keymap_size);
		}

		wl_resource_set_implementation(resource, &interface, nullptr, nullptr);
		
		wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, keymap_fd, keymap_size);

		if (version >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
			wl_keyboard_send_repeat_info(resource, 25, 1000);

		data.keyboards[wl_client].resource = resource;

		WM::Client::Bind::Keyboard(wl_client, resource);

		return resource;
	}

	void Button(wl_client* client, uint32_t button, bool released)
	{
		if (state == nullptr)
			return;

		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		auto stateKey = released ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED;

		xkb_state_update_key(state, button + 8, (xkb_key_direction)stateKey);

		if (!surface)
			return;

		xkb_mod_mask_t depressed = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
		xkb_mod_mask_t latched = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
		xkb_mod_mask_t locked = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
		xkb_mod_mask_t group = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);

		wl_array* states = new wl_array();
		wl_array_init(states);

		for (auto resource : WM::Client::Get::All::Keyboards(client))
		{
			wl_keyboard_send_enter((wl_resource*)resource, NextSerialNum(), surface, states);

			wl_keyboard_send_modifiers((wl_resource*)resource, NextSerialNum(), depressed, latched, locked, group);
			wl_keyboard_send_key((wl_resource*)resource, NextSerialNum(), time, button, stateKey);

			wl_keyboard_send_leave((wl_resource*)resource, NextSerialNum(), surface);
		}
		
		wl_array_release(states);
		delete states;
	}

	void ChangeWindow(wl_client* client1, wl_resource* surface1, wl_client* client2, wl_resource* surface2)
	{
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		//if (Surface::data.surfaces.contains(surface1))
		//	for (auto resource : WM::Client::Get::All::Keyboards(client1))
		//	{
		//		wl_keyboard_send_leave((wl_resource*)resource, NextSerialNum(), surface1);
		//	}
//
		//for (auto resource : WM::Client::Get::All::Keyboards(client2))
		//{
		//	wl_array* states = new wl_array();
		//	wl_array_init(states);
//
		//	wl_keyboard_send_enter((wl_resource*)resource, NextSerialNum(), surface2, states);
//
		//	wl_array_release(states);
		//	delete states;
		//}

		surface = surface2;
	}
}