#include "shell_surface.hpp"
#include "surface.hpp"
#include "pointer.hpp"
#include "seat.hpp"
#include <spdlog/spdlog.h>
#include "wm/input.hpp"

namespace Awning::Protocols::WL::Shell_Surface
{
	const struct wl_shell_surface_interface interface = {
		.pong           = Interface::Pong,
		.move           = Interface::Move,
		.resize         = Interface::Resize,
		.set_toplevel   = Interface::Set_Toplevel,
		.set_transient  = Interface::Set_Transient,
		.set_fullscreen = Interface::Set_Fullscreen,
		.set_popup      = Interface::Set_Popup,
		.set_maximized  = Interface::Set_Maximized,
		.set_title      = Interface::Set_Title,
		.set_class      = Interface::Set_Class
	};
	std::unordered_map<wl_resource*, Instance> instances;

	namespace Interface
	{
		void Pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
		}

		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			//((Input::Seat*)Seat::global.instances[seat].seat)->Lock(Input::Action::Move);
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{
			Input::WindowSide side;

			switch (edges)
			{
			case WL_SHELL_SURFACE_RESIZE_TOP         : side = Input::WindowSide::TOP         ; break;
			case WL_SHELL_SURFACE_RESIZE_BOTTOM      : side = Input::WindowSide::BOTTOM      ; break;
			case WL_SHELL_SURFACE_RESIZE_LEFT        : side = Input::WindowSide::LEFT        ; break;
			case WL_SHELL_SURFACE_RESIZE_TOP_LEFT    : side = Input::WindowSide::TOP_LEFT    ; break;
			case WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT : side = Input::WindowSide::BOTTOM_LEFT ; break;
			case WL_SHELL_SURFACE_RESIZE_RIGHT       : side = Input::WindowSide::RIGHT       ; break;
			case WL_SHELL_SURFACE_RESIZE_TOP_RIGHT   : side = Input::WindowSide::TOP_RIGHT   ; break;
			case WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT: side = Input::WindowSide::BOTTOM_RIGHT; break;
			}

			//((Input::Seat*)Seat::global.instances[seat].seat)->Lock(Input::Action::Resize, side);
		}

		void Set_Toplevel(struct wl_client* client, struct wl_resource* resource)
		{
		}

		void Set_Transient(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
		{
		}

		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, uint32_t method, uint32_t framerate, struct wl_resource* output)
		{
		}

		void Set_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
		{
		}

		void Set_Maximized(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output)
		{
		}

		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title)
		{
		}

		void Set_Class(struct wl_client* client, struct wl_resource* resource, const char* class_)
		{
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* surface) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &wl_shell_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		instances[resource] = Instance();

		instances[resource].surface = surface;
		instances[resource].window = Window::Create(wl_client);
		Surface::instances[surface].window = instances[resource].window;

		Window::Manager::Manage(instances[resource].window);
		Window::Manager::Raise(instances[resource].window);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		auto surface = instances[resource].surface; 
		Surface::instances[surface].window = nullptr;
		
		instances[resource].window->Mapped(false);
		instances[resource].window->Texture(nullptr);
		Window::Destory(instances[resource].window);
		instances.erase(resource);
	}
}