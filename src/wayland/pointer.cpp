#include "pointer.hpp"
#include "shell_surface.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wm/drawable.hpp"

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

	void Moved(double x, double y)
	{
		//Log::Function::Called("Wayland::Pointer");

		wl_resource* active = nullptr;
		for (auto& [resource, window] : WM::Drawable::drawables)
		{
			if (Surface::data.surfaces.find(window.surface) != Surface::data.surfaces.end())
			{
				if (*window.xPosition <= x) 
					if (*window.yPosition <= y)
						if (*window.xPosition + *window.xDimension > x)
							if (*window.yPosition + *window.yDimension > y)
				{
					active = resource;
					break;
				}
			}
		}

		if (!data.moveMode)
		{
			if (active)
			{
				auto& shell = WM::Drawable::drawables[active];
				auto& surface = Surface::data.surfaces[shell.surface];
				auto pointer = &data.pointers[surface.client];

				pointer->xPos = x;
				pointer->yPos = y;

				x = (x - *shell.xPosition);
				y = (y - *shell.yPosition);

				pointer->xLPos = x;
				pointer->yLPos = y;
			}

			int xPoint = wl_fixed_from_double(x);
			int yPoint = wl_fixed_from_double(y);
	
			if (active != data.pre_shell)
			{
				if (data.pre_shell != nullptr)
				{
					auto& pre_shell = WM::Drawable::drawables[data.pre_shell];
					
					if (Surface::data.surfaces.find(pre_shell.surface) != Surface::data.surfaces.end())
					{
						auto& surface = Surface::data.surfaces[pre_shell.surface];
						auto resource = data.pointers[surface.client].resource;
						wl_pointer_send_leave(resource, NextSerialNum(), pre_shell.surface);
					}
				}
				
				if (active != nullptr)
				{
					auto& shell = WM::Drawable::drawables[active];
					auto& surface = Surface::data.surfaces[shell.surface];
					auto resource = data.pointers[surface.client].resource;
					wl_pointer_send_enter(resource, NextSerialNum(), shell.surface, xPoint, yPoint);
					wl_pointer_send_frame(resource);
				}
				
				data.pre_shell = active;
			}
			else if (active != nullptr)
			{
				auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
				auto& shell = WM::Drawable::drawables[active];
				auto& surface = Surface::data.surfaces[shell.surface];
				auto resource = data.pointers[surface.client].resource;
				wl_pointer_send_motion(resource, time, xPoint, yPoint);
				wl_pointer_send_frame(resource);
				Surface::HandleFrameCallbacks();
			}
		}
		else
		{
			auto& shell = WM::Drawable::drawables[data.pre_shell];
			auto& surface = Surface::data.surfaces[shell.surface];
			auto& pointer = data.pointers[surface.client];

			*shell.xPosition = x - *shell.xPosition;
			*shell.yPosition = y - *shell.yPosition;
		}
	}

	void Button(uint32_t button, bool released)
	{
		//Log::Function::Called("Wayland::Pointer");

		if (!data.moveMode)
		{
			if (data.pre_shell != nullptr)
			{
				auto& shell = WM::Drawable::drawables[data.pre_shell];
				auto& surface = Surface::data.surfaces[shell.surface];
				auto resource = data.pointers[surface.client].resource;

				auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000000;
				wl_pointer_send_button(resource, NextSerialNum(), time, button, released);
			}
		}
		else
		{
			data.moveMode = false;
			data.pre_shell = nullptr;
		}	
	}

	void MoveMode()
	{
		data.moveMode = true;

		auto& pre_shell = WM::Drawable::drawables[data.pre_shell];
		auto& surface = Surface::data.surfaces[pre_shell.surface];
		auto resource = data.pointers[surface.client].resource;
		wl_pointer_send_leave(resource, NextSerialNum(), pre_shell.surface);
	}
}