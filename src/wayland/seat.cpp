#include "seat.hpp"
#include "shell_surface.hpp"
#include "surface.hpp"
#include "log.hpp"

#include <chrono>

uint32_t NextSerialNum();

namespace Awning::Wayland::Pointer
{
	const struct wl_pointer_interface interface = {
		.set_cursor = [](struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y) 
		{
			Log::Function::Called("Wayland::Pointer.interface.set_cursor");
		},
		.release    = [](struct wl_client *client, struct wl_resource *resource) 
		{
			Log::Function::Called("Wayland::Pointer.interface.release");
		},
	};

	Data data;

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id)
	{
		Log::Function::Called("Wayland::Pointer");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_pointer_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, nullptr);

		data.pointers[wl_client].resource = resource;
	}

	void Moved(double x, double y)
	{
		//Log::Function::Called("Wayland::Pointer");

		wl_resource* active = nullptr;
		for (auto& [resource, shell] : Shell_Surface::data.shells)
		{
			auto& window = Surface::data.surfaces[shell.surface];

			if (shell.xPosition <= x) 
				if (shell.yPosition <= y)
					if (shell.xPosition + window.xDimension > x)
						if (shell.yPosition + window.yDimension > y)
			{
				active = resource;
				break;
			}
		}

		if (active)
		{
			auto& shell = Shell_Surface::data.shells[active];
			auto& surface = Surface::data.surfaces[shell.surface];

			x = (x - shell.xPosition);
			y = (y - shell.yPosition);
		}

		int xPoint = wl_fixed_from_double(x);
		int yPoint = wl_fixed_from_double(y);

		if (active != data.pre_shell)
		{
			if (data.pre_shell != nullptr)
			{
				auto& pre_shell = Shell_Surface::data.shells[data.pre_shell];
				auto resource = data.pointers[pre_shell.client].resource;
				wl_pointer_send_leave(resource, NextSerialNum(), pre_shell.surface);
			}
			
			if (active != nullptr)
			{
				auto& shell = Shell_Surface::data.shells[active];
				auto resource = data.pointers[shell.client].resource;
				wl_pointer_send_enter(resource, NextSerialNum(), shell.surface, xPoint, yPoint);
			}
			
			data.pre_shell = active;
		}
		else if (active != nullptr)
		{
			auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
			auto& shell = Shell_Surface::data.shells[active];
			auto resource = data.pointers[shell.client].resource;
			wl_pointer_send_motion(resource, time, xPoint, yPoint);
		}
		//wl_pointer_send_frame(data.resource);
	}

	void Button(uint32_t button, bool released)
	{
		//Log::Function::Called("Wayland::Pointer");

		if (data.pre_shell != nullptr)
		{
			auto& shell = Shell_Surface::data.shells[data.pre_shell];
			auto resource = data.pointers[shell.client].resource;

			auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
			wl_pointer_send_button(resource, NextSerialNum(), time, button, released);
		}
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