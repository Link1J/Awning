#include "pointer.hpp"
#include "shell_surface.hpp"
#include "surface.hpp"
#include <spdlog/spdlog.h>

#include <linux/input.h>

#include <chrono>
#include <iostream>

#include "wm/input.hpp"

uint32_t NextSerialNum();

namespace Awning::Protocols::WL::Pointer
{
	const struct wl_pointer_interface interface = {
		.set_cursor = Interface::Set_Cursor,
		.release    = Interface::Release   ,
	};

	Data data;

	namespace Interface
	{
		void Set_Cursor(struct wl_client* client, struct wl_resource* resource, uint32_t serial, struct wl_resource* surface, int32_t hotspot_x, int32_t hotspot_y)
		{
			if (data.inUse)
			{
				data.instances[data.inUse].inUse = false;	
			}

			if (data.instances[resource].seat)
			{
				Input::Seat* seat = (Input::Seat*)data.instances[resource].seat;

				Window::Manager::Offset(seat->pointer.window, hotspot_x, hotspot_y);
				Window::Manager::Resize(seat->pointer.window, 0, 0);

				auto texture = Awning::Protocols::WL::Surface::data.surfaces[surface].texture;

				if (!texture)
					return;
					
				seat->pointer.window->Texture(texture);
				Window::Manager::Resize(seat->pointer.window, texture->width, texture->height);

				data.instances[resource].inUse = true;
				data.inUse = resource;
			}
		}

		void Release(struct wl_client* client, struct wl_resource* resource)
		{
			Awning::Protocols::WL::Pointer::Destroy(resource);
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, void* seat)
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_pointer_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.instances[resource].client  = wl_client;
		data.instances[resource].version = version  ;
		data.instances[resource].seat    = seat     ;

		Client::Bind::Pointer(wl_client, resource);

		Input::Seat::FunctionSet functions = {
			.moved  = Moved   ,
			.button = Button  ,
			.scroll = Axis    ,
			.enter  = Enter   ,
			.leave  = Leave   ,
			.frame  = Frame   ,
			.data   = resource,
		};

		((Input::Seat*)seat)->AddFunctions(1, wl_client, functions);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.instances.contains(resource))
			return;

		if (resource == data.inUse)
		{
			((Input::Seat*)data.instances[resource].seat)->pointer.window->Texture(nullptr);
			data.inUse = nullptr;
		}

		((Input::Seat*)data.instances[resource].seat)->RemoveFunctions(1, data.instances[resource].client, resource);
		Client::Unbind::Pointer(data.instances[resource].client, resource);
		data.instances.erase(resource);
	}
	
	void Enter(void* data, void* object, int x, int y)
	{
		if (!data  ) return;
		if (!object) return;

		auto version = Pointer::data.instances[(wl_resource*)data].version;
		if (version < WL_POINTER_ENTER_SINCE_VERSION) return;
			
		int xPoint = wl_fixed_from_int(x);
		int yPoint = wl_fixed_from_int(y);

		wl_pointer_send_enter((wl_resource*)data, NextSerialNum(), (wl_resource*)object, xPoint, yPoint);
	}

	void Leave(void* data, void* object)
	{
		if (!data  ) return;
		if (!object) return;

		auto version = Pointer::data.instances[(wl_resource*)data].version;
		if (version < WL_POINTER_LEAVE_SINCE_VERSION) return;

		wl_pointer_send_leave((wl_resource*)data, NextSerialNum(), (wl_resource*)object);
	}

	void Moved(void* data, int x, int y)
	{
		if (!data) return;

		auto version = Pointer::data.instances[(wl_resource*)data].version;
		if (version < WL_POINTER_MOTION_SINCE_VERSION) return;

		uint32_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		int xPoint = wl_fixed_from_int(x);
		int yPoint = wl_fixed_from_int(y);

		wl_pointer_send_motion((wl_resource*)data, time, xPoint, yPoint);
	}

	void Button(void* data, uint32_t button, bool state)
	{
		if (!data) return;

		auto version = Pointer::data.instances[(wl_resource*)data].version;
		if (version < WL_POINTER_BUTTON_SINCE_VERSION) return;

		uint32_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;

		wl_pointer_send_button((wl_resource*)data, NextSerialNum(), time, button, state);
	}

	void Axis(void* data, uint32_t axis, int step)
	{
		if (!data) return;

		auto version = Pointer::data.instances[(wl_resource*)data].version;
		if (version < WL_POINTER_AXIS_SINCE_VERSION) return;

		uint32_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
		
		step = wl_fixed_from_int(step);
		
		wl_pointer_send_axis((wl_resource*)data, time, axis, step);
	}

	void Frame(void* data)
	{
		auto version = Pointer::data.instances[(wl_resource*)data].version;
		if (version < WL_POINTER_FRAME_SINCE_VERSION) return;

		wl_pointer_send_frame((wl_resource*)data);
	}
}