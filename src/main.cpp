#include "backends/X11.hpp"

#include <wayland-server.h>
#include "protocols/xdg-shell-protocol.h"

#include <sys/signal.h>

#include "wayland/compositor.hpp"
#include "wayland/seat.hpp"
#include "wayland/output.hpp"
#include "wayland/shell.hpp"
#include "wayland/surface.hpp"

#include "xdg/wm_base.hpp"

#include <iostream>
#include <unordered_set>

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);

int on_term_signal(int signal_number, void* data);
int sigchld_handler(int signal_number, void* data);

namespace Awning
{
	namespace Server
	{
		struct Data
		{
			wl_display* display;
			wl_event_loop* event_loop;
			wl_event_source* signals[4];
			wl_list child_process_list;
			wl_protocol_logger* logger; 
		};
	}	
	Server::Data server;
};

std::unordered_set<wl_resource*> openWindows;

int main()
{
	X11::Start();

	Awning::server.display = wl_display_create();
	Awning::server.event_loop = wl_display_get_event_loop(Awning::server.display);

	//wl_list_init(&Awning::server.child_process_list);

	//Awning::server.signals[0] = wl_event_loop_add_signal(Awning::server.event_loop, SIGTERM, on_term_signal, Awning::server.display);
	//Awning::server.signals[1] = wl_event_loop_add_signal(Awning::server.event_loop, SIGINT , on_term_signal, Awning::server.display);
	//Awning::server.signals[2] = wl_event_loop_add_signal(Awning::server.event_loop, SIGQUIT, on_term_signal, Awning::server.display);
	//Awning::server.signals[3] = wl_event_loop_add_signal(Awning::server.event_loop, SIGCHLD, sigchld_handler, NULL);
	
	wl_display_add_protocol_logger(Awning::server.display, ProtocolLogger, nullptr);    
	const char* socket = wl_display_add_socket_auto(Awning::server.display);
	std::cout << "Wayland Socket: " << socket << std::endl;

	{
		using namespace Awning;
		using namespace Awning::Wayland;

		Compositor::data.global = wl_global_create(server.display, &wl_compositor_interface, 1, nullptr, Compositor::Bind);
		Seat      ::data.global = wl_global_create(server.display, &wl_seat_interface      , 1, nullptr, Seat      ::Bind);
		Output    ::data.global = wl_global_create(server.display, &wl_output_interface    , 3, nullptr, Output    ::Bind);
		Shell     ::data.global = wl_global_create(server.display, &wl_shell_interface     , 1, nullptr, Shell     ::Bind);

		wl_display_init_shm(server.display);
	}

	{
		using namespace Awning;
		using namespace Awning::XDG;

		//WM_Base::data.global = wl_global_create(server.display, &xdg_wm_base_interface, 1, nullptr, WM_Base::Bind);
	}
	
	while (1)
	{
		X11::Poll();

		wl_event_loop_dispatch(Awning::server.event_loop, 0);
		wl_display_flush_clients(Awning::server.display);

		auto data = X11::Data();

		for (auto& resource : openWindows)
		{
			auto& window = Awning::Wayland::Surface::data.surfaces[resource];

			for (int x = 0; x < window.xDimension; x++)
				for (int y = 0; y < window.yDimension; y++)
				{
					if (x >= X11::Width() || y >= X11::Height())
						continue;

					int windowOffset = (x + y * window.xDimension) * 4;
					int framebOffset = (x + y * X11::Width()     ) * 4;

					data[framebOffset + 0] = window.data[windowOffset + 2];
					data[framebOffset + 1] = window.data[windowOffset + 1];
					data[framebOffset + 2] = window.data[windowOffset + 0];
					data[framebOffset + 3] = window.data[windowOffset + 3];
				}
		}
	}

	wl_display_destroy(Awning::server.display);
}

int on_term_signal(int signal_number, void *data)
{
	return 0;
}

int sigchld_handler(int signal_number, void *data)
{
	return 0;
}

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message)
{
	const char* direction_strings[] = { 
		"REQUEST", 
		"EVENT  "
	};

	//std::cout << "[" << direction_strings[direction] << "] " << message->resource->object.interface->name << ": " << message->message->name << "\n";
}