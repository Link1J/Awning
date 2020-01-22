#include "shell_surface.hpp"
#include "surface.hpp"
#include "log.hpp"

#include "wm/drawable.hpp"

#include <unordered_map>

extern std::unordered_map<wl_resource*, Awning::WM::Drawable::Data> drawables;

namespace Awning::Wayland::Shell_Surface
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
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Toplevel(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Transient(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, uint32_t method, uint32_t framerate, struct wl_resource* output)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Maximized(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}

		void Set_Class(struct wl_client* client, struct wl_resource* resource, const char* class_)
		{
			Log::Function::Called("Wayland::Shell_Surface::Interface");
		}
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* surface) 
	{
		Log::Function::Called("Wayland::Shell_Surface");

		struct wl_resource* resource = wl_resource_create(wl_client, &wl_shell_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.shells[resource] = Data::Instance();

		data.shells[resource].surface = surface;
		data.shells[resource].xPosition = 0;
		data.shells[resource].yPosition = 0;

		drawables[resource].xPosition  = &         data.shells  [resource].xPosition ;
		drawables[resource].yPosition  = &         data.shells  [resource].yPosition ;
		drawables[resource].xDimension = &Surface::data.surfaces[surface ].xDimension;
		drawables[resource].yDimension = &Surface::data.surfaces[surface ].yDimension;
		drawables[resource].data       = &Surface::data.surfaces[surface ].data      ;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Wayland::Shell_Surface");

		drawables.erase(resource);
		data.shells.erase(resource);
	}
}