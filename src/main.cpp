#include "backends/X11.hpp"

#include <wayland-server.h>
#include "protocols/xdg-shell-protocol.h"

#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "wayland/compositor.hpp"
#include "wayland/seat.hpp"
#include "wayland/output.hpp"
#include "wayland/shell.hpp"
#include "wayland/surface.hpp"
#include "wayland/shell_surface.hpp"

#include "xdg/wm_base.hpp"
#include "xdg/decoration.hpp"

#include "wm/drawable.hpp"

#include <iostream>
#include <unordered_map>

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

uint32_t lastSerialNum = 1;

uint32_t NextSerialNum()
{
	lastSerialNum++;
	return lastSerialNum;
}

int main(int argc, char* argv[])
{
	bool noX = false;

	for(int a = 0; a < argc; a++)
	{
		auto arg = std::string(argv[a]);
		if (arg == "-noX")
		{
			noX = true;
		}
	}

	if (!noX)
	{
    	int pid;
    	int status, ret;
    	char* XWaylandArgs [] = { "Xwayland", ":1", NULL };

		pid = fork();

		if (pid == 0) 
		{
			ret = execvp(XWaylandArgs[0], XWaylandArgs);
			printf("Xwayland did not launch! %d %s\n", ret, strerror(errno));
			return ret;
    	}
	}

	Awning::server.display = wl_display_create(); 
	const char* socket = wl_display_add_socket_auto(Awning::server.display);
	std::cout << "Wayland Socket: " << socket << std::endl;

	X11::Start();

	Awning::server.event_loop = wl_display_get_event_loop(Awning::server.display);
	wl_display_add_protocol_logger(Awning::server.display, ProtocolLogger, nullptr);   

	{
		using namespace Awning;
		using namespace Awning::Wayland;

		Compositor::data.global = wl_global_create(server.display, &wl_compositor_interface, 3, nullptr, Compositor::Bind);
		Seat      ::data.global = wl_global_create(server.display, &wl_seat_interface      , 4, nullptr, Seat      ::Bind);
		Output    ::data.global = wl_global_create(server.display, &wl_output_interface    , 3, nullptr, Output    ::Bind);
		Shell     ::data.global = wl_global_create(server.display, &wl_shell_interface     , 1, nullptr, Shell     ::Bind);

		wl_display_init_shm(server.display);
	}

	{
		using namespace Awning;
		using namespace Awning::XDG;
		using namespace Awning::ZXDG;

		WM_Base           ::data.global = wl_global_create(server.display, &xdg_wm_base_interface               , 1, nullptr, WM_Base           ::Bind);
		Decoration_Manager::data.global = wl_global_create(server.display, &zxdg_decoration_manager_v1_interface, 1, nullptr, Decoration_Manager::Bind);
	}
	
	while (1)
	{
		X11::Poll();

		wl_event_loop_dispatch(Awning::server.event_loop, 0);
		wl_display_flush_clients(Awning::server.display);

		auto data = X11::Data();

		for (auto& [resource, drawable] : Awning::WM::Drawable::drawables)
		{
			if (!drawable.data)
				continue;
			if (!*drawable.data)
				continue;

			for (int x = -1; x < *drawable.xDimension + 1; x++)
				for (int y = -10; y < *drawable.yDimension + 1; y++)
				{
					if ((*drawable.xPosition + x) <  0            )
						continue;
					if ((*drawable.yPosition + y) <  0            )
						continue;
					if ((*drawable.xPosition + x) >= X11::Width ())
						continue;
					if ((*drawable.yPosition + y) >= X11::Height())
						continue;

					int windowOffset = (x + y * (*drawable.xDimension)) * 4;
					int framebOffset = ((*drawable.xPosition + x) + (*drawable.yPosition + y) * X11::Width()) * 4;

					if (x < *drawable.xDimension && y < *drawable.yDimension && x >= 0 && y >= 0)
					{
						data[framebOffset + 0] = (*drawable.data)[windowOffset + 2];
						data[framebOffset + 1] = (*drawable.data)[windowOffset + 1];
						data[framebOffset + 2] = (*drawable.data)[windowOffset + 0];
						data[framebOffset + 3] = (*drawable.data)[windowOffset + 3];
					}
					else
					{
						if (drawable.needsFrame)
						{
							data[framebOffset + 0] = 0xFF;
							data[framebOffset + 1] = 0xFF;
							data[framebOffset + 2] = 0xFF;
							data[framebOffset + 3] = 0xFF;
						}
					}
				}
		}

		Awning::Wayland::Surface::HandleFrameCallbacks();
		X11::Draw();
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