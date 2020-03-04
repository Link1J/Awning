#include "surface.hpp"
#include "toplevel.hpp"
#include "popup.hpp"
#include "log.hpp"

#include "protocols/wl/surface.hpp"
#include "wm/manager.hpp"

#include "backends/manager.hpp"

#include <unordered_map>

uint32_t NextSerialNum();

namespace Awning::Protocols::XDG::Surface
{
	const struct xdg_surface_interface interface = {
		.destroy             = Interface::Destroy,
		.get_toplevel        = Interface::Get_Toplevel,
		.get_popup           = Interface::Get_Popup,
		.set_window_geometry = Interface::Set_Window_Geometry,
		.ack_configure       = Interface::Ack_Configure,
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::XDG::Surface::Interface");
			Surface::Destroy(resource);
		}

		void Get_Toplevel(struct wl_client* client, struct wl_resource* resource, uint32_t id)
		{
			Log::Function::Called("Protocols::XDG::Surface::Interface");
			TopLevel::Create(client, 1, id, resource);
		}

		void Get_Popup(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* parent, struct wl_resource* positioner)
		{
			Log::Function::Called("Protocols::XDG::Surface::Interface");
			Popup::Create(client, 1, id, resource, parent, positioner);
		}

		void Set_Window_Geometry(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			Log::Function::Called("Protocols::XDG::Surface::Interface");
			
			if (!data.surfaces[resource].window)
				return;

			auto window = data.surfaces[resource].window;
			
			if (x != 0 || y != 0)
				WM::Window::Manager::Offset(window, x, y);
			
			if (window->XSize() == 0 || window->YSize() == 0)
				WM::Window::Manager::Resize(window, width, height);

			auto display = Backend::GetDisplays()[0];
			auto [sx, sy] = WM::Output::Get::Mode::Resolution(display.output, display.mode);

			if (window->XPos() == INT32_MIN)
				WM::Window::Manager::Move(window, sx/2. - window->XSize()/2., window->YPos());
			if (window->YPos() == INT32_MIN)
				WM::Window::Manager::Move(window, window->XPos(), sy/2. - window->YSize()/2.);
		}

		void Ack_Configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
			Log::Function::Called("Protocols::XDG::Surface::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface) 
	{
		Log::Function::Called("Protocols::XDG::Surface");

		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_surface_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.surfaces[resource].surface_wl = surface;
		
		WL::Surface::data.surfaces[surface].type = 1;
		WL::Surface::data.surfaces[surface].shell = resource;

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::XDG::Surface");

		data.surfaces.erase(resource);
	}
}