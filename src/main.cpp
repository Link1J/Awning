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

#include "renderers/software.hpp"

#include <fmt/printf.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gbm.h>

#include <iostream>
#include <unordered_map>

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
int XWM_Start(int signal_number, void *data);
void LaunchXwayland(int signal_number);
void on_term_signal(int signal_number);
void client_created(struct wl_listener *listener, void *data);
void loadEGLProc(void* proc_ptr, const char* name);
void launchApp(const char** argv);

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
				EGLContext context;
				EGLSurface surface;
			} egl;
		};
		Data data;
	}
};

typedef void (*PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, EGLImage image);

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

PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

extern int tty_fd;

uint32_t lastSerialNum = 1;

uint32_t NextSerialNum()
{
	lastSerialNum++;
	return lastSerialNum;
}

static void eglLog(EGLenum error, const char *command, EGLint msg_type, EGLLabelKHR thread, EGLLabelKHR obj, const char *msg) {
	std::cout << fmt::format("[EGL] command: {}, error: 0x{:X}, message: \"{}\"", command, error, msg);
}

int main(int argc, char* argv[])
{
	bool noX = true;
	Awning::Backend::API api_output = Awning::Backend::API::FBDEV;
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
			api_input  = Awning::Backend::API::libinput;
		}
		if (arg == "-x11")
		{
			api_output = Awning::Backend::API::X11;
			api_input  = Awning::Backend::API::X11;
		}
		if (arg == "-drm")
		{
			api_output = Awning::Backend::API::DRM;
			api_input  = Awning::Backend::API::libinput;
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

	int32_t fd = open("/dev/dri/renderD128", O_RDWR);
	struct gbm_device* gbm = gbm_create_device(fd);

	//Awning::Server::data.egl.display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
	Awning::Server::data.egl.display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, gbm, NULL);

	eglInitialize(Awning::Server::data.egl.display, &Awning::Server::data.egl.major, &Awning::Server::data.egl.minor);

	loadEGLProc(&eglBindWaylandDisplayWL     , "eglBindWaylandDisplayWL"     );
	loadEGLProc(&eglUnbindWaylandDisplayWL   , "eglUnbindWaylandDisplayWL"   );
	loadEGLProc(&eglQueryWaylandBufferWL     , "eglQueryWaylandBufferWL"     );
	loadEGLProc(&eglCreateImageKHR           , "eglCreateImageKHR"           );
	loadEGLProc(&eglDestroyImageKHR          , "eglDestroyImageKHR"          );
	loadEGLProc(&glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES");
	loadEGLProc(&eglDebugMessageControlKHR   , "eglDebugMessageControlKHR"   );

	static const EGLAttrib debug_attribs[] = {
		EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
		EGL_DEBUG_MSG_ERROR_KHR   , EGL_TRUE,
		EGL_DEBUG_MSG_WARN_KHR    , EGL_TRUE,
		EGL_DEBUG_MSG_INFO_KHR    , EGL_TRUE,
		EGL_NONE,
	};

	eglDebugMessageControlKHR(eglLog, debug_attribs);

	eglBindWaylandDisplayWL(Awning::Server::data.egl.display, Awning::Server::data.display);

	std::cout << "EGL Vendor    : " << eglQueryString(Awning::Server::data.egl.display, EGL_VENDOR ) << "\n";
	std::cout << "EGL Version   : " << eglQueryString(Awning::Server::data.egl.display, EGL_VERSION) << "\n";

	EGLint attribs[] = { 
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE 
	};
	EGLConfig config;
	EGLint num_configs_returned;
	eglChooseConfig(Awning::Server::data.egl.display, attribs, &config, 1, &num_configs_returned);

	eglBindAPI(EGL_OPENGL_ES_API);

	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

	Awning::Server::data.egl.context = eglCreateContext(Awning::Server::data.egl.display, config, EGL_NO_CONTEXT, contextAttribs);
	
	eglMakeCurrent(Awning::Server::data.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, Awning::Server::data.egl.context);

	std::cout << "GL Vendor     : " << glGetString(GL_VENDOR                  ) << "\n";
	std::cout << "GL Renderer   : " << glGetString(GL_RENDERER                ) << "\n";
	std::cout << "GL Version    : " << glGetString(GL_VERSION                 ) << "\n";
	//std::cout << "GLSL Version  : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
	
	Awning::Wayland::Compositor        ::Add(Awning::Server::data.display);
	Awning::Wayland::Seat              ::Add(Awning::Server::data.display);
	Awning::Wayland::Output            ::Add(Awning::Server::data.display);
	Awning::Wayland::Shell             ::Add(Awning::Server::data.display);
	Awning::XDG    ::WM_Base           ::Add(Awning::Server::data.display);
	//Awning::ZXDG   ::Decoration_Manager::Add(Awning::Server::data.display);

	wl_display_init_shm(Awning::Server::data.display);

	Awning::Renderers::Software::Init();

	if (pid != 0)
	{
		kill(pid, SIGUSR2);
	}

    const char* launchArgs1[] = { "falkon", "-platform", "wayland", NULL };
    const char* launchArgs2[] = { "weston-terminal", NULL };

	launchApp(launchArgs1);
	//launchApp(launchArgs2);
	
	while(1)
	{
		Awning::Wayland::Surface::HandleFrameCallbacks();

		Awning::Backend::Poll();
		Awning::Backend::Hand();

		wl_event_loop_dispatch(Awning::Server::data.event_loop, 0);
		wl_display_flush_clients(Awning::Server::data.display);

		Awning::WM::X::EventLoop();

		Awning::Renderers::Software::Draw();

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
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);

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

void launchApp(const char** argv)
{
	int pid = fork();
	if (pid == 0) 
	{
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);

		int ret = execvp(argv[0], (char**)argv);
		printf("%s did not launch! %d %s\n", argv[0], ret, strerror(errno));
		exit(ret);
	}
}