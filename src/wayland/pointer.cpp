#include "pointer.hpp"
#include "shell_surface.hpp"
#include "surface.hpp"
#include "log.hpp"

#include <linux/input.h>

#include <chrono>
#include <iostream>

uint32_t NextSerialNum();

namespace Awning::Wayland::Pointer
{
	const struct wl_pointer_interface interface = {
		.set_cursor = [](struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y) 
		{
			Log::Function::Called("Wayland::Pointer::interface.set_cursor");
			data.pointers[resource].texture = Awning::Wayland::Surface::data.surfaces[surface].texture;
		},
		.release    = [](struct wl_client *client, struct wl_resource *resource) 
		{
			Log::Function::Called("Wayland::Pointer::interface.release");
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
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.pointers[resource].client = wl_client;

		WM::Client::Bind::Pointer(wl_client, resource);
	}

	void Destroy(struct wl_resource* resource)
	{
		WM::Client::Unbind::Pointer(data.pointers[resource].client, resource);
		data.pointers.erase(resource);
	}

	void Enter(wl_client* client, wl_resource* surface, double x, double y) 
	{
		if (!surface)
			return;
			
		int xPoint = wl_fixed_from_double(x);
		int yPoint = wl_fixed_from_double(y);

		for (auto resource : WM::Client::Get::All::Pointers(client))
			wl_pointer_send_enter((wl_resource*)resource, NextSerialNum(), surface, xPoint, yPoint);
	}

	void Leave(wl_client* client, wl_resource* surface) 
	{
		if (!surface)
			return;
		
		for (auto resource : WM::Client::Get::All::Pointers(client))
			wl_pointer_send_leave((wl_resource*)resource, NextSerialNum(), surface);
	}

	void Moved(wl_client* client, double x, double y) 
	{
		int xPoint = wl_fixed_from_double(x);
		int yPoint = wl_fixed_from_double(y);
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		
		for (auto resource : WM::Client::Get::All::Pointers(client))
			wl_pointer_send_motion((wl_resource*)resource, time, xPoint, yPoint);
	}

	void Button(wl_client* client, uint32_t button, bool pressed) 
	{
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		
		for (auto resource : WM::Client::Get::All::Pointers(client))
			wl_pointer_send_button((wl_resource*)resource, NextSerialNum(), time, button, pressed);
	}

	void Axis(wl_client* client, uint32_t axis, float value) 
	{
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		int step = wl_fixed_from_double(value);
		
		for (auto resource : WM::Client::Get::All::Pointers(client))
			wl_pointer_send_axis((wl_resource*)resource, time, axis, step);
	}

	void Frame(wl_client* client)
	{
		for (auto resource : WM::Client::Get::All::Pointers(client))
			wl_pointer_send_frame((wl_resource*)resource);
	}
}