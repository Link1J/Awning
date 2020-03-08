#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xfixes.h>
#include <linux/input.h>

#include "protocols/wl/pointer.hpp"
#include "protocols/wl/keyboard.hpp"

#include "wm/manager.hpp"
#include "wm/output.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <thread>
#include <iostream>

#include <fmt/format.h>

#include <spdlog/spdlog.h>

#include "X11.hpp"
#include "manager.hpp"

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct WindowData
{	
	EGLSurface         surface            ;
	bool 		       resized     = false;
	Awning::Texture    framebuffer        ;
	Awning::Output::ID id                 ;
	GLuint             texture            ;
};

static Display*		                            display    ;
static EGLDisplay                               egl_display;
static EGLContext                               egl_context;
static std::unordered_map<::Window, WindowData> windows    ;

static Atom WM_DELETE_WINDOW;

#include "renderers/egl.hpp"

static const char* vertexShaderCode = R"(
#version 300 es
precision mediump float;

out vec4 color;

void main()
{
	float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);
	float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);

	gl_Position = vec4(-1.0+x*2.0,-1.0+y*2.0,0.0,1.0);
	color       = vec4(     x    , 1.0-y    ,0.0,1.0);
}
)";

static const char* pixelShaderCode = R"(
#version 300 es
precision mediump float;

uniform sampler2D texture0;

in vec4 color;
out vec4 FragColor;

void main()
{
	FragColor = texture2D(texture0, color.xy).bgra;
}
)";

namespace Awning::Backend::X11
{
	void CreateBackingTexture(::Window window)
	{
		WindowData& data = windows[window];

		eglMakeCurrent(egl_display, data.surface, data.surface, egl_context);

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &data.texture);
		glBindTexture(GL_TEXTURE_2D, data.texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data.framebuffer.width, data.framebuffer.height, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		data.framebuffer.buffer.pointer = new uint8_t[data.framebuffer.size];
	}

	void ReleaseBackingTexture(::Window window)
	{
		WindowData& data = windows[window];

		eglMakeCurrent(egl_display, data.surface, data.surface, egl_context);

		glDeleteTextures(1, &data.texture);

		if (data.framebuffer.buffer.pointer)
		{
			delete data.framebuffer.buffer.pointer;
			data.framebuffer.buffer.pointer = nullptr;
		}
	}
}

static void eglLog(EGLenum error, const char *command, EGLint msg_type, EGLLabelKHR thread, EGLLabelKHR obj, const char *msg) {
	spdlog::error("[X EGL] command: {}, error: 0x{:X}, message: \"{}\"\n", command, error, msg);
}

static GLuint vertexShader, pixelShader, program;

void Awning::Backend::X11::Start()
{
	int width = 1024, height = 576;

	int screen;
	XSetWindowAttributes attribs;
 
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);
	auto root = RootWindow(display, screen);

	EGLint attr[] = {
		EGL_BUFFER_SIZE, 16,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLConfig  ecfg;
	EGLint     num_config;

	egl_display = eglGetDisplay((EGLNativeDisplayType)display);
	eglInitialize(egl_display, NULL, NULL);
	eglChooseConfig(egl_display, attr, &ecfg, 1, &num_config);
	egl_context = eglCreateContext(egl_display, ecfg, EGL_NO_CONTEXT, ctxattr);
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);

	PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControlKHR;
	Renderers::EGL::loadEGLProc(&eglDebugMessageControlKHR   , "eglDebugMessageControlKHR"   );

	static const EGLAttrib debug_attribs[] = {
		EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
		EGL_DEBUG_MSG_ERROR_KHR   , EGL_TRUE,
		EGL_DEBUG_MSG_WARN_KHR    , EGL_TRUE,
		EGL_DEBUG_MSG_INFO_KHR    , EGL_TRUE,
		EGL_NONE,
	};

	eglDebugMessageControlKHR(eglLog, debug_attribs);

	auto extensions = std::string(eglQueryString(egl_display, EGL_EXTENSIONS));

	//std::cout << "X EGL Vendor  : " << eglQueryString(egl_display, EGL_VENDOR ) << "\n";
	//std::cout << "X EGL Version : " << eglQueryString(egl_display, EGL_VERSION) << "\n";
	//std::cout << "EGL Extensions: " << extensions                               << "\n";
	//std::cout << "X GL Vendor   : " << glGetString(GL_VENDOR                  ) << "\n";
	//std::cout << "X GL Renderer : " << glGetString(GL_RENDERER                ) << "\n";
	//std::cout << "X GL Version  : " << glGetString(GL_VERSION                 ) << "\n";
	//std::cout << "X GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
	
	attribs.event_mask = StructureNotifyMask|ButtonPressMask|KeyPressMask|PointerMotionMask|ButtonReleaseMask|KeyReleaseMask;
	
	WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);

	glClearColor(1, 1, 1, 1);
	Renderers::EGL::CreateShader(vertexShader, GL_VERTEX_SHADER, vertexShaderCode);
	Renderers::EGL::CreateShader(pixelShader, GL_FRAGMENT_SHADER, pixelShaderCode);
	Renderers::EGL::CreateProgram(program, vertexShader, pixelShader);
	glUseProgram(program);

	int xOffset = 0;

	int displays = atoi(getenv("AWNING_DISPLAYS_COUNT") ? getenv("AWNING_DISPLAYS_COUNT") : "1");

	for (int a = 1; a <= displays; a++)
	{
		WindowData data;
		::Window window = XCreateWindow(display, root, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &attribs);

		XClassHint classHint {
			(char*)"Awning",
			(char*)"Awning"
		};
		std::string displayName = fmt::format("X11-{}", a);

		XSetClassHint(display, window, &classHint);
		XStoreName(display, window, displayName.c_str());
		XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
		XMapWindow(display, window);

		data.surface = eglCreateWindowSurface(egl_display, ecfg, window, NULL);

		data.framebuffer = Awning::Texture {
    	    .size         = (uintptr_t)width * height * 4,
    	    .bitsPerPixel = 32,
    	    .bytesPerLine = (uintptr_t)width * 4,
    	    .red          = { 
				.size   = 8,
				.offset = 16
			},
    	    .green        = { 
				.size   = 8,
				.offset = 8
			},
    	    .blue         = { 
				.size   = 8,
				.offset = 0
			},
    	    .alpha        = { 
				.size   = 8,
				.offset = 24
			},
    	    .width        = (uintptr_t)width,
    	    .height       = (uintptr_t)height
    	};

		data.id = Output::Create();
		Output::Set::NumberOfModes(data.id, 1);

		Output::Set::Manufacturer(data.id, "X.Org Foundation"     );
		Output::Set::Model       (data.id, "11.0"                 );
		Output::Set::Size        (data.id, 0, 0                   );
		Output::Set::Position    (data.id, xOffset, 0             );
		Output::Set::Name        (data.id, displayName            );
		Output::Set::Description (data.id, "X.Org Foundation 11.0");
		
		Output::Set::Mode::Resolution (data.id, 0, data.framebuffer.width, data.framebuffer.height);
		Output::Set::Mode::RefreshRate(data.id, 0, 0                                              );
		Output::Set::Mode::Prefered   (data.id, 0, true                                           );
		Output::Set::Mode::Current    (data.id, 0, true                                           );

		windows[window] = data;

		CreateBackingTexture(window);

		xOffset += data.framebuffer.width;
	}
}

uint32_t XorgMouseToLinuxInputMouse(uint32_t button)
{
	if (button == Button1) return BTN_LEFT;
	if (button == Button2) return BTN_MIDDLE;
	if (button == Button3) return BTN_RIGHT;
	return BTN_EXTRA;
}

void Awning::Backend::X11::Hand()
{
	XEvent event;

	while(XPending(display))
	{
    	XNextEvent(display, &event);

		if (event.type == ConfigureNotify)
		{
			WindowData& data = windows[event.xconfigure.window];
			if (data.framebuffer.width == event.xconfigure.width && data.framebuffer.height == event.xconfigure.height)
				continue;

			data.framebuffer.width  = event.xconfigure.width ;
			data.framebuffer.height = event.xconfigure.height;

			data.framebuffer.size = data.framebuffer.width * data.framebuffer.height * 4;
			data.framebuffer.bytesPerLine = data.framebuffer.width * 4;

			ReleaseBackingTexture(event.xconfigure.window);
			CreateBackingTexture (event.xconfigure.window);

			Output::Set::Mode::Resolution(data.id, 0, data.framebuffer.width, data.framebuffer.height);
		}
		else if (event.type == ClientMessage)
		{
			ReleaseBackingTexture(event.xclient.window);
			//eglDestroySurface(egl_display, windows[event.xclient.window].surface);
			Output::Destory(windows[event.xclient.window].id);
			windows.erase(event.xclient.window);
			XDestroyWindow(display, event.xclient.window);
		}
		else if (event.type == MotionNotify)
		{
			auto [px, py] = Output::Get::Position(windows[event.xmotion.window].id);
			Awning::WM::Manager::Handle::Input::Mouse::Moved(px + event.xmotion.x, py + event.xmotion.y);			
		}
		else if (event.type == ButtonPress)
		{
			if (event.xbutton.button > Button3)
			{
				if (event.xbutton.button == Button4)
				{
					Awning::WM::Manager::Handle::Input::Mouse::Scroll(0, -15);
				}
				if (event.xbutton.button == Button5)
				{
					Awning::WM::Manager::Handle::Input::Mouse::Scroll(0, 15);
				}
			}
			else
			{
				uint32_t button = XorgMouseToLinuxInputMouse(event.xbutton.button);
				Awning::WM::Manager::Handle::Input::Mouse::Pressed(button);
			}
		}
		else if (event.type == ButtonRelease)
		{
			uint32_t button = XorgMouseToLinuxInputMouse(event.xbutton.button);
			Awning::WM::Manager::Handle::Input::Mouse::Released(button);
		}
		else if (event.type == KeyPress)
		{
			Awning::WM::Manager::Handle::Input::Keyboard::Pressed(event.xkey.keycode - 8);
		}
		else if (event.type == KeyRelease)
		{
			Awning::WM::Manager::Handle::Input::Keyboard::Released(event.xkey.keycode - 8);
		}
	}
}

void Awning::Backend::X11::Draw()
{
	for (auto [window, data] : windows)
	{
		eglMakeCurrent(egl_display, data.surface, data.surface, egl_context);

		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, data.framebuffer.width, data.framebuffer.height);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, data.texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data.framebuffer.width, data.framebuffer.height, GL_RGBA, GL_UNSIGNED_BYTE, data.framebuffer.buffer.pointer);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		eglSwapBuffers(egl_display, data.surface);
	}
}

void Awning::Backend::X11::Poll()
{
}

Awning::Backend::Displays Awning::Backend::X11::GetDisplays()
{
	Displays displays;

	for (auto [window, data] : windows)
	{
		if (data.id == -1)
			continue;

		Display display;
		display.output  = data.id         ;
		display.texture = data.framebuffer;
		display.mode    = 0               ;
		displays.push_back(display);
	}

	return displays;
}