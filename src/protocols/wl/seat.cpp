#include "seat.hpp"
#include "pointer.hpp"
#include "keyboard.hpp"
#include <spdlog/spdlog.h>
#include "wm/input.hpp"

namespace Awning::Protocols::WL::Seat
{
	const struct wl_seat_interface interface = {
		.get_pointer  = Interface::Get_Pointer,
		.get_keyboard = Interface::Get_Keyboard,
		.get_touch    = Interface::Get_Touch,
		.release      = Interface::Release,
	};

	Global global;

	namespace Interface
	{
		void Get_Pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Pointer::Create(client, wl_resource_get_version(resource), id, global.instances[resource].seat);
		}

		void Get_Keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Keyboard::Create(client, wl_resource_get_version(resource), id, global.instances[resource].seat);
		}

		void Get_Touch(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
		}

		void Release(struct wl_client* client, struct wl_resource* resource)
		{
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_seat_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		Input::Seat* seat = (Input::Seat*)data;
		int capabilities = (int)seat->Capabilities();

		wl_seat_send_capabilities(resource, 
			(capabilities & (int)Input::Seat::Capability::Mouse    ? WL_SEAT_CAPABILITY_POINTER  : 0) |
			(capabilities & (int)Input::Seat::Capability::Keyboard ? WL_SEAT_CAPABILITY_KEYBOARD : 0) |
			(capabilities & (int)Input::Seat::Capability::Touch    ? WL_SEAT_CAPABILITY_TOUCH    : 0)
		);

		if (version >= WL_SEAT_NAME_SINCE_VERSION)
			wl_seat_send_name(resource, seat->Name().c_str());

		global.instances[resource].seat = data;
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_seat_interface, 5, data, Bind);
	}
}