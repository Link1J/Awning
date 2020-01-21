#include "seat.hpp"
#include "log.hpp"

#include <chrono>

namespace Awning::Wayland::Pointer
{
	const struct wl_pointer_interface interface = {
		.set_cursor = [](struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y) 
		{
			Log::Function::Called("Wayland::Pointer.interface");
		},
		.release    = [](struct wl_client *client, struct wl_resource *resource) 
		{
			Log::Function::Called("Wayland::Pointer.interface");
		},
	};

	Data data;

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Wayland::Pointer");

		if (data.resource)
			return;

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_pointer_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, nullptr);

		data.resource = resource;
	}

	void Moved(int x, int y)
	{
		//Log::Function::Called("Wayland::Pointer");

		if (!data.resource)
			return;

		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		wl_pointer_send_motion(data.resource, time, wl_fixed_from_int(x), wl_fixed_from_int(y));
	}
}

namespace Awning::Wayland::Seat
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
			Log::Function::Called("Wayland::Seat::Interface");
			Pointer::Create(client, wl_resource_get_version(resource), id);
		}

		void Get_Keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("Wayland::Seat::Interface");
		}

		void Get_Touch(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("Wayland::Seat::Interface");
		}

		void Release(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Wayland::Seat::Interface");
		}
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id) 
	{
		Log::Function::Called("Wayland::Seat");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_seat_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, data, nullptr);

		wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER);
		//WL_SEAT_CAPABILITY_KEYBOARD
	}
}