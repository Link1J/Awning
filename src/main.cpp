#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11

#include "backends/manager.hpp"

#include <wayland-server.h>
#include "protocols/xdg-shell-protocol.h"

#include <linux/kd.h>

#include <sys/ioctl.h>
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
#include "wayland/pointer.hpp"

#include "xdg/wm_base.hpp"
#include "xdg/decoration.hpp"

#include "wm/x/wm.hpp"
#include "wm/window.hpp"
#include "wm/client.hpp"
#include "wm/manager.hpp"

#include "log.hpp"

#include <fmt/printf.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <iostream>
#include <unordered_map>

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
int XWM_Start(int signal_number, void *data);
void LaunchXwayland(int signal_number);
void on_term_signal(int signal_number);
void client_created(struct wl_listener *listener, void *data);
void loadEGLProc(void* proc_ptr, const char* name);

// -------------------------------------------------------------------
// --- Reversed iterable

template <typename T>
struct reversion_wrapper { T& iterable; };
template <typename T>
auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }
template <typename T>
auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }
template <typename T>
reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }


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
			wl_listener client_listener;

			struct {
				EGLDisplay display;
				EGLint major, minor;
			} egl;
		};
		Data data;
	}
};

PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
PFNEGLQUERYWAYLANDBUFFERWL eglQueryWaylandBufferWL;
PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL;
PFNEGLUNBINDWAYLANDDISPLAYWL eglUnbindWaylandDisplayWL;
PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC eglSwapBuffersWithDamage; // KHR or EXT
PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT;
PFNEGLQUERYDMABUFMODIFIERSEXTPROC eglQueryDmaBufModifiersEXT;
PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA;
PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA;
PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControlKHR;

extern int tty_fd;

uint32_t lastSerialNum = 1;

uint32_t NextSerialNum()
{
	lastSerialNum++;
	return lastSerialNum;
}

int main(int argc, char* argv[])
{
	bool noX = true;
	Awning::Backend::API api_output = Awning::Backend::API::DRM;
	Awning::Backend::API api_input  = Awning::Backend::API::libinput;

	for(int a = 0; a < argc; a++)
	{
		auto arg = std::string(argv[a]);
		if (arg == "-noX")
		{
			noX = true;
		}
		if (arg == "-fbdev")
		{
			api_output = Awning::Backend::API::FBDEV;
			api_input  = Awning::Backend::API::EVDEV;
		}
		if (arg == "-x11")
		{
			api_output = Awning::Backend::API::X11;
			api_input  = Awning::Backend::API::X11;
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

	Awning::Server::data.display = wl_display_create(); 
	const char* socket = wl_display_add_socket_auto(Awning::Server::data.display);
	std::cout << "Wayland Socket: " << socket << std::endl;

	Awning::Backend::Init(api_output, api_input);

	Awning::Server::data.client_listener.notify = client_created;

	Awning::Server::data.event_loop = wl_display_get_event_loop(Awning::Server::data.display);
	wl_display_add_protocol_logger(Awning::Server::data.display, ProtocolLogger, nullptr);
	wl_display_add_client_created_listener(Awning::Server::data.display, &Awning::Server::data.client_listener);

	Awning::Server::data.sigusr1 = wl_event_loop_add_signal(Awning::Server::data.event_loop, SIGUSR1, XWM_Start, nullptr);

	loadEGLProc(&eglGetPlatformDisplayEXT , "eglGetPlatformDisplayEXT" );

	eglBindAPI(EGL_OPENGL_ES_API);	
	Awning::Server::data.egl.display = eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);

	eglInitialize(Awning::Server::data.egl.display, &Awning::Server::data.egl.major, &Awning::Server::data.egl.minor);

	loadEGLProc(&eglBindWaylandDisplayWL  , "eglBindWaylandDisplayWL"  );
	loadEGLProc(&eglUnbindWaylandDisplayWL, "eglUnbindWaylandDisplayWL");
	loadEGLProc(&eglQueryWaylandBufferWL  , "eglQueryWaylandBufferWL"  );

	std::cout << "EGL Vendor    : " << eglQueryString(Awning::Server::data.egl.display, EGL_VENDOR) << "\n";
	std::cout << "EGL Version   : " << eglQueryString(Awning::Server::data.egl.display, EGL_VERSION) << "\n";

	//eglBindWaylandDisplayWL(Awning::Server::data.egl.display, Awning::Server::data.display);
	
	Awning::Wayland::Compositor        ::Add(Awning::Server::data.display);
	Awning::Wayland::Seat              ::Add(Awning::Server::data.display);
	Awning::Wayland::Output            ::Add(Awning::Server::data.display);
	Awning::Wayland::Shell             ::Add(Awning::Server::data.display);
	Awning::XDG    ::WM_Base           ::Add(Awning::Server::data.display);
	Awning::ZXDG   ::Decoration_Manager::Add(Awning::Server::data.display);

	wl_display_init_shm(Awning::Server::data.display);

	if (pid != 0)
	{
		kill(pid, SIGUSR2);
	}

	pid = fork();
	if (pid == 0) 
	{
    	const char* XWaylandArgs [] = { "falkon", "-platform", "wayland", NULL };

		int fd = open("/dev/null", O_RDWR);
		//dup2(fd, STDOUT_FILENO);
		//dup2(fd, STDERR_FILENO);

		int ret = execvp(XWaylandArgs[0], (char**)XWaylandArgs);
		printf("falkon did not launch! %d %s\n", ret, strerror(errno));
		exit(ret);
	}
	
	while(1)
	{
		Awning::Wayland::Surface::HandleFrameCallbacks();

		Awning::Backend::Poll();
		Awning::Backend::Hand();

		wl_event_loop_dispatch(Awning::Server::data.event_loop, 0);
		wl_display_flush_clients(Awning::Server::data.display);

		Awning::WM::X::EventLoop();

		auto data = Awning::Backend::Data();
		auto list = Awning::WM::Manager::Window::Get();
		void* client = 0;

		memset(data.buffer.u8, 0xEE, data.size);

		for (auto& window : reverse(list))
		{
			auto texture = window->Texture();

			if (!window->Mapped())
				continue;
			if (!texture)
				continue;

			auto winPosX  = window->XPos   ();
			auto winPosY  = window->YPos   ();
			auto winSizeX = window->XSize  ();
			auto winSizeY = window->YSize  ();
			auto winOffX  = window->XOffset();
			auto winOffY  = window->YOffset();

			for (int x = -Frame::Size::left; x < winSizeX + winOffX + Frame::Size::right; x++)
				for (int y = -Frame::Size::top; y < winSizeY + winOffY + Frame::Size::bottom; y++)
				{
					if ((winPosX + x - winOffX) <  0          )
						continue;
					if ((winPosY + y - winOffY) <  0          )
						continue;
					if ((winPosX + x - winOffX) >= data.width )
						continue;
					if ((winPosY + y - winOffY) >= data.height)
						continue;

					int windowOffset = (x) * (texture->bitsPerPixel / 8)
									 + (y) *  texture->bytesPerLine    ;

					int framebOffset = (winPosX + x - winOffX) * (data.bitsPerPixel / 8)
									 + (winPosY + y - winOffY) *  data.bytesPerLine    ;

					uint8_t red, green, blue, alpha;

					if (x < winSizeX + winOffX && y < winSizeY + winOffY && x >= 0 && y >= 0)
					{
						if (texture->buffer.u8 != nullptr && windowOffset < texture->size)
						{
							red   = texture->buffer.u8[windowOffset + (texture->red  .offset / 8)];
							green = texture->buffer.u8[windowOffset + (texture->green.offset / 8)];
							blue  = texture->buffer.u8[windowOffset + (texture->blue .offset / 8)];
							alpha = texture->buffer.u8[windowOffset + (texture->alpha.offset / 8)];
						}
						else
						{
							red   = 0x00;
							green = 0x00;
							blue  = 0x00;
							alpha = 0xFF;
						}						
					}
					else
					{
						if (window->Frame())
						{
							red   = 0x00;
							green = 0xFF;
							blue  = 0xFF;
							alpha = 0xFF;
						}
						else 
						{
							red   = 0x00;
							green = 0x00;
							blue  = 0x00;
							alpha = 0x00;
						}
					}

					if (alpha > 0)
					{
						uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
						uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
						uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

						buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
						buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
						buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
					}
				}
		}

		if (Awning::Wayland::Pointer::data.window)
		{
			auto texture = Awning::Wayland::Pointer::data.window->Texture();

			if (texture)
			{
				auto winPosX  = Awning::Wayland::Pointer::data.window->XPos   ();
				auto winPosY  = Awning::Wayland::Pointer::data.window->YPos   ();
				auto winSizeX = Awning::Wayland::Pointer::data.window->XSize  ();
				auto winSizeY = Awning::Wayland::Pointer::data.window->YSize  ();
				auto winOffX  = Awning::Wayland::Pointer::data.window->XOffset();
				auto winOffY  = Awning::Wayland::Pointer::data.window->YOffset();

				for (int x = 0; x < winSizeX; x++)
					for (int y = 0; y < winSizeY; y++)
					{
						if ((winPosX + x) <  0          )
							continue;
						if ((winPosY + y) <  0          )
							continue;
						if ((winPosX + x) >= data.width )
							continue;
						if ((winPosY + y) >= data.height)
							continue;

						int windowOffset = (x) * (texture->bitsPerPixel / 8)
										 + (y) *  texture->bytesPerLine    ;

						int framebOffset = (winPosX + x - winOffX) * (data.bitsPerPixel / 8)
										 + (winPosY + y - winOffY) *  data.bytesPerLine    ;

						uint8_t red, green, blue, alpha;

						if (texture->buffer.u8 != nullptr)
						{
							red   = texture->buffer.u8[windowOffset + (texture->red  .offset / 8)];
							green = texture->buffer.u8[windowOffset + (texture->green.offset / 8)];
							blue  = texture->buffer.u8[windowOffset + (texture->blue .offset / 8)];
							alpha = texture->buffer.u8[windowOffset + (texture->alpha.offset / 8)];
						}
						else
						{
							red   = 0x00;
							green = 0xFF;
							blue  = 0x00;
							alpha = 0xFF;
						}

						if (alpha > 0 && framebOffset > 0 && framebOffset < data.size)
						{
							uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
							uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
							uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

							buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
							buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
							buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
						}
					}
			}
			else
			{
				auto winPosX  = Awning::Wayland::Pointer::data.window->XPos   ();
				auto winPosY  = Awning::Wayland::Pointer::data.window->YPos   ();
				auto winSizeX = 3                                               ;
				auto winSizeY = 3                                               ;
				auto winOffX  = 0                                               ;
				auto winOffY  = 0                                               ;

				for (int x = 0; x < winSizeX; x++)
					for (int y = 0; y < winSizeY; y++)
					{
						if ((winPosX + x - winOffX) <  0          )
							continue;
						if ((winPosY + y - winOffY) <  0          )
							continue;
						if ((winPosX + x - winOffX) >= data.width )
							continue;
						if ((winPosY + y - winOffY) >= data.height)
							continue;


						int framebOffset = (winPosX + x) * (data.bitsPerPixel / 8)
										 + (winPosY + y) *  data.bytesPerLine    ;

						uint8_t red, green, blue, alpha;

						red   = 0x00;
						green = 0xFF;
						blue  = 0x00;
						alpha = 0xFF;

						uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
						uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
						uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

						buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
						buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
						buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
					}
			}			
		}

		Awning::Backend::Draw();
	}

	wl_display_destroy(Awning::Server::data.display);
	ioctl(tty_fd, KDSETMODE, KD_TEXT);
	ioctl(tty_fd, KDSKBMODE, K_RAW);
}

void on_term_signal(int signal_number)
{
}

int XWM_Start(int signal_number, void *data)
{
	Log::Function::Called("");
	//Awning::WM::X::Init();
	signal(SIGUSR1, on_term_signal);
	return 0;
}

void LaunchXwayland(int signal_number)
{
	Log::Function::Called("");

    const char* XWaylandArgs [] = { "Xwayland", ":1", NULL };

	int fd = open("/dev/null", O_RDWR);
	//dup2(fd, STDOUT_FILENO);
	//dup2(fd, STDERR_FILENO);

	int ret = execvp(XWaylandArgs[0], (char**)XWaylandArgs);
	printf("Xwayland did not launch! %d %s\n", ret, strerror(errno));
	exit(ret);
}

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message)
{
	const char* direction_strings[] = { 
		"REQUEST", 
		"EVENT  "
	};

	if (direction == 1)
		return;

	//std::cout << "[" << direction_strings[direction] << "] " << message->resource->object.interface->name << ": " << message->message->name << "\n";
}

void client_created(struct wl_listener* listener, void* data)
{
	Log::Function::Called("");

	Awning::WM::Client::Create(data);

	if (!Awning::WM::X::xWaylandClient)
		Awning::WM::X::xWaylandClient = (wl_client*)data;
}

void loadEGLProc(void* proc_ptr, const char* name)
{
	void* proc = (void*)eglGetProcAddress(name);
	if (proc == NULL) {
		Log::Report::Error(fmt::format("eglGetProcAddress({}) failed", name));
		abort();
	}
	*(void**)proc_ptr = proc;
}

/*void GetSockAddress()
{
	const char *dir = getenv("XDG_RUNTIME_DIR");
	if (!dir) {
		dir = "/tmp";
	}
	if (path_size <= snprintf(ipc_sockaddr->sun_path, path_size,
			"%s/awning-ipc.%i.%i.sock", dir, getuid(), getpid())) {
	}
}*/