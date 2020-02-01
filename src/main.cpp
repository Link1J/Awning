#include "backends/manager.hpp"

#include <wayland-server.h>
#include "protocols/xdg-shell-protocol.h"

#include <sys/signal.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "wayland/compositor.hpp"
#include "wayland/seat.hpp"
#include "wayland/output.hpp"
#include "wayland/shell.hpp"
#include "wayland/surface.hpp"
#include "wayland/shell_surface.hpp"

#include "xdg/wm_base.hpp"
#include "xdg/decoration.hpp"

#include "wm/drawable.hpp"

#include "wm/x/wm.hpp"

#include "log.hpp"

#include <iostream>
#include <unordered_map>

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
int XWM_Start(int signal_number, void *data);
void LaunchXwayland(int signal_number);

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
			wl_event_source* sigusr1;
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
	Awning::Backend::API api = Awning::Backend::API::X11;

	for(int a = 0; a < argc; a++)
	{
		auto arg = std::string(argv[a]);
		if (arg == "-noX")
		{
			noX = true;
		}
		if (arg == "-fbdev")
		{
			api = Awning::Backend::API::FBDEV;
		}
		if (arg == "-x11")
		{
			api = Awning::Backend::API::X11;
		}
	}

    int pid;
    int status, ret;

	if (!noX)
	{
		pid = fork();
	}

	if (!noX && pid == 0) 
	{
		signal(SIGUSR1, SIG_IGN);
		signal(SIGUSR2, LaunchXwayland);
		pause();
   		exit(-1);
	}

	Awning::server.display = wl_display_create(); 
	const char* socket = wl_display_add_socket_auto(Awning::server.display);
	std::cout << "Wayland Socket: " << socket << std::endl;

	Awning::Backend::Init(api);

	Awning::server.event_loop = wl_display_get_event_loop(Awning::server.display);
	wl_display_add_protocol_logger(Awning::server.display, ProtocolLogger, nullptr);

	Awning::server.sigusr1 = wl_event_loop_add_signal(Awning::server.event_loop, SIGUSR1, XWM_Start, nullptr);

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

	if (pid != 0)
	{
		kill(pid, SIGUSR2);
	}
	
	while(1)
	{
		Awning::Backend::Poll();

		wl_event_loop_dispatch(Awning::server.event_loop, 0);
		wl_display_flush_clients(Awning::server.display);

		Awning::WM::X::EventLoop();

		auto data = Awning::Backend::Data();

		for (auto& [resource, drawable] : Awning::WM::Drawable::drawables)
		{
			if (!drawable.data)
				continue;
			if (!*drawable.data)
				continue;

			for (int x = -1; x < *drawable.xDimension + 1; x++)
				for (int y = -10; y < *drawable.yDimension + 1; y++)
				{
					if ((*drawable.xPosition + x) <  0          )
						continue;
					if ((*drawable.yPosition + y) <  0          )
						continue;
					if ((*drawable.xPosition + x) >= data.width )
						continue;
					if ((*drawable.yPosition + y) >= data.height)
						continue;

					int windowOffset = (x + y * (*drawable.xDimension)) * 4;
					int framebOffset = (*drawable.xPosition + x) * (data.bitsPerPixel / 8)
									 + (*drawable.yPosition + y) * data.bytesPerLine;

					uint8_t red, green, blue;

					if (x < *drawable.xDimension && y < *drawable.yDimension && x >= 0 && y >= 0)
					{
						red   = (*drawable.data)[windowOffset + 2];
						green = (*drawable.data)[windowOffset + 1];
						blue  = (*drawable.data)[windowOffset + 0];
					}
					else
					{
						if (drawable.needsFrame)
						{
							red   = 0xFF;
							green = 0xFF;
							blue  = 0xFF;
						}
					}

					data.buffer.u8[framebOffset + (data.red  .offset / 8)] = red  ;
					data.buffer.u8[framebOffset + (data.green.offset / 8)] = green;
					data.buffer.u8[framebOffset + (data.blue .offset / 8)] = blue ;
				}
		}

		Awning::Wayland::Surface::HandleFrameCallbacks();
		Awning::Backend::Draw();
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

int XWM_Start(int signal_number, void *data)
{
	Log::Function::Called("");
	//Awning::WM::X::Init();
	signal(SIGUSR1, SIG_IGN);
	return 0;
}

void LaunchXwayland(int signal_number)
{
	Log::Function::Called("");

    char* XWaylandArgs [] = { "Xwayland", ":1", NULL };

	int fd = open("/dev/null", O_RDWR);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);

	int ret = execvp(XWaylandArgs[0], XWaylandArgs);
	printf("Xwayland did not launch! %d %s\n", ret, strerror(errno));
	exit(ret);
}

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message)
{
	const char* direction_strings[] = { 
		"REQUEST", 
		"EVENT  "
	};

	//std::cout << "[" << direction_strings[direction] << "] " << message->resource->object.interface->name << ": " << message->message->name << "\n";
}