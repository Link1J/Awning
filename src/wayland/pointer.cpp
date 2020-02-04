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
		wl_resource_set_implementation(resource, &interface, nullptr, nullptr);

		data.pointers[wl_client].resource = resource;
		data.pointers[wl_client].xLPos = -1;
		data.pointers[wl_client].yLPos = -1;
	}

	void Enter(wl_client* client, wl_resource* surface, double x, double y) 
	{
		if (!data.pointers.contains(client))
			return;
		int xPoint = wl_fixed_from_double(x);
		int yPoint = wl_fixed_from_double(y);
		wl_pointer_send_enter(data.pointers[client].resource, NextSerialNum(), surface, xPoint, yPoint);
	}

	void Leave(wl_client* client, wl_resource* surface) 
	{
		if (!data.pointers.contains(client))
			return;
		wl_pointer_send_leave(data.pointers[client].resource, NextSerialNum(), surface);
	}

	void Moved(wl_client* client, double x, double y) 
	{
		if (!data.pointers.contains(client))
			return;
		int xPoint = wl_fixed_from_double(x);
		int yPoint = wl_fixed_from_double(y);
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		wl_pointer_send_motion(data.pointers[client].resource, time, xPoint, yPoint);
	}

	void Button(wl_client* client, uint32_t button, bool pressed) 
	{
		if (!data.pointers.contains(client))
			return;
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		wl_pointer_send_button(data.pointers[client].resource, NextSerialNum(), time, button, pressed);
	}

	void Axis(wl_client* client, uint32_t axis, float value) 
	{
		if (!data.pointers.contains(client))
			return;
		auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		int step = wl_fixed_from_double(value);
		wl_pointer_send_axis(data.pointers[client].resource, time, axis, step);
	}

	void Frame(wl_client* client)
	{
		if (!data.pointers.contains(client))
			return;
		wl_pointer_send_frame(data.pointers[client].resource);
	}
}