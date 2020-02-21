#include "seat.hpp"
#include "pointer.hpp"
#include "keyboard.hpp"
#include "log.hpp"

namespace Awning::Protocols::WL::Seat
{
	const struct wl_seat_interface interface = {
		.get_pointer  = Interface::Get_Pointer,
		.get_keyboard = Interface::Get_Keyboard,
		.get_touch    = Interface::Get_Touch,
		.release      = Interface::Release,
	};

	Data data;

	namespace Interface
	{
		void Get_Pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("Protocols::WL::Seat::Interface");
			Pointer::Create(client, wl_resource_get_version(resource), id);
		}

		void Get_Keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("Protocols::WL::Seat::Interface");
			Keyboard::Create(client, wl_resource_get_version(resource), id);
		}

		void Get_Touch(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("Protocols::WL::Seat::Interface");
		}

		void Release(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WL::Seat::Interface");
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Protocols::WL::Seat");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_seat_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		wl_seat_send_name(resource, "Maybe Working Input.");
		wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER|WL_SEAT_CAPABILITY_KEYBOARD);
	}

	wl_global* Add(struct wl_display* display, void* data)
	{
		return wl_global_create(display, &wl_seat_interface, 3, data, Bind);
	}
}