#include "shell_surface.hpp"
#include "surface.hpp"
#include "pointer.hpp"
#include "log.hpp"

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

	Data data;

	namespace Interface
	{
		void Pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
			WM::Manager::Handle::Input::Lock(WM::Manager::Handle::Input::MOVE);
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");

			WM::Manager::Handle::Input::WindowSide side;
			switch (edges)
			{
			case WL_SHELL_SURFACE_RESIZE_TOP         : side = WM::Manager::Handle::Input::TOP         ; break;
			case WL_SHELL_SURFACE_RESIZE_BOTTOM      : side = WM::Manager::Handle::Input::BOTTOM      ; break;
			case WL_SHELL_SURFACE_RESIZE_LEFT        : side = WM::Manager::Handle::Input::LEFT        ; break;
			case WL_SHELL_SURFACE_RESIZE_TOP_LEFT    : side = WM::Manager::Handle::Input::TOP_LEFT    ; break;
			case WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT : side = WM::Manager::Handle::Input::BOTTOM_LEFT ; break;
			case WL_SHELL_SURFACE_RESIZE_RIGHT       : side = WM::Manager::Handle::Input::RIGHT       ; break;
			case WL_SHELL_SURFACE_RESIZE_TOP_RIGHT   : side = WM::Manager::Handle::Input::TOP_RIGHT   ; break;
			case WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT: side = WM::Manager::Handle::Input::BOTTOM_RIGHT; break;
			}

			WM::Manager::Handle::Input::Lock(WM::Manager::Handle::Input::RESIZE, side);
		}

		void Set_Toplevel(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Set_Transient(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, uint32_t method, uint32_t framerate, struct wl_resource* output)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Set_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Set_Maximized(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}

		void Set_Class(struct wl_client* client, struct wl_resource* resource, const char* class_)
		{
			Log::Function::Called("Protocols::WL::Shell_Surface::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* surface) 
	{
		Log::Function::Called("Protocols::WL::Shell_Surface");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_shell_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.shells[resource] = Data::Instance();

		data.shells[resource].surface = surface;
		//data.shells[resource].window = WM::Window::Create(wl_client);
		//Surface::data.surfaces[surface].window = data.shells[resource].window;

		WM::Manager::Window::Raise(data.shells[resource].window);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::WL::Shell_Surface");

		if (!data.shells.contains(resource))
			return;

		auto surface = data.shells[resource].surface; 
		Surface::data.surfaces[surface].window = nullptr;
		
		data.shells[resource].window->Mapped(false);
		data.shells[resource].window->Texture(nullptr);
		WM::Window::Destory(data.shells[resource].window);
		data.shells.erase(resource);
	}
}