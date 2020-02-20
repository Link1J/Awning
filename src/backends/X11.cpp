#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xfixes.h>
#include <linux/input.h>

#include "wayland/pointer.hpp"
#include "wayland/keyboard.hpp"

#include "wm/manager.hpp"
#include "wm/output.hpp"

#include "log.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <thread>
#include <iostream>

#include <fmt/format.h>

#include "X11.hpp"
#include "manager.hpp"

#include  <GLES2/gl2.h>
#include  <EGL/egl.h>

struct WindowData
{	
	EGLSurface             surface            ;
	bool 		           resized     = false;
	Awning::WM::Texture    framebuffer        ;
	Awning::WM::Output::ID id                 ;
	GLuint                 texture            ;
};

static Display*		                          display    ;
static EGLDisplay                             egl_display;
static EGLContext                             egl_context;
static std::unordered_map<Window, WindowData> windows    ;

void CreateShader(GLuint& shader, GLenum type, const char* code, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());
void CreateProgram(GLuint& program, GLuint vertexShader, GLuint pixelShader, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());

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
#extension GL_OES_EGL_image_external : require

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
	void CreateBackingTexture(Window window)
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

	void ReleaseBackingTexture(Window window)
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

GLuint vertexShader, pixelShader, program;

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

	Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
	Atom _NET_WM_STATE_ADD = XInternAtom(display, "_NET_WM_STATE_ADD", False);
	Atom max_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	Atom max_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

	std::cout << "X EGL Vendor  : " << eglQueryString(egl_display, EGL_VENDOR ) << "\n";
	std::cout << "X EGL Version : " << eglQueryString(egl_display, EGL_VERSION) << "\n";
	std::cout << "X GL Vendor   : " << glGetString(GL_VENDOR                  ) << "\n";
	std::cout << "X GL Renderer : " << glGetString(GL_RENDERER                ) << "\n";
	std::cout << "X GL Version  : " << glGetString(GL_VERSION                 ) << "\n";
	std::cout << "X GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
	
	attribs.event_mask = StructureNotifyMask|ButtonPressMask|KeyPressMask|PointerMotionMask|ButtonReleaseMask|KeyReleaseMask;
	
	glClearColor(1, 1, 1, 1);
	CreateShader(vertexShader, GL_VERTEX_SHADER, vertexShaderCode);
	CreateShader(pixelShader, GL_FRAGMENT_SHADER, pixelShaderCode);
	CreateProgram(program, vertexShader, pixelShader);
	glUseProgram(program);

	int xOffset = 0;

	for (int a = 0; a < 3; a++)
	{
		WindowData data;
		Window window = XCreateWindow(display, root, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &attribs);

		XEvent xev;
		memset(&xev, 0, sizeof(xev));
		xev.type = ClientMessage;
		xev.xclient.window = window;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = _NET_WM_STATE_ADD;
		xev.xclient.data.l[1] = max_horz;
		xev.xclient.data.l[2] = max_vert;

		XSendEvent(display, root, False, SubstructureNotifyMask, &xev);

		XStoreName(display, window, "Awning (X11 Backend)");
		XMapWindow(display, window);

		data.surface = eglCreateWindowSurface(egl_display, ecfg, window, NULL);

		data.framebuffer = Awning::WM::Texture {
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

		data.id = WM::Output::Create();
		WM::Output::Set::NumberOfModes(data.id, 1);

		WM::Output::Set::Manufacturer(data.id, "X.Org Foundation");
		WM::Output::Set::Model       (data.id, "11.0"            );
		WM::Output::Set::Size        (data.id, 0, 0              );
		WM::Output::Set::Position    (data.id, xOffset, 0        );
		
		WM::Output::Set::Mode::Resolution (data.id, 0, data.framebuffer.width, data.framebuffer.height);
		WM::Output::Set::Mode::RefreshRate(data.id, 0, 0                                              );
		WM::Output::Set::Mode::Prefered   (data.id, 0, true                                           );
		WM::Output::Set::Mode::Current    (data.id, 0, true                                           );

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
			if (data.framebuffer.width == event.xconfigure.width || data.framebuffer.height == event.xconfigure.height)
				continue;

			data.framebuffer.width  = event.xconfigure.width ;
			data.framebuffer.height = event.xconfigure.height;

			data.framebuffer.size = data.framebuffer.width * data.framebuffer.height * 4;
			data.framebuffer.bytesPerLine = data.framebuffer.width * 4;

			ReleaseBackingTexture(event.xconfigure.window);
			CreateBackingTexture (event.xconfigure.window);

			WM::Output::Set::Mode::Resolution(data.id, 0, data.framebuffer.width, data.framebuffer.height);
		}
		else if (event.type == ClientMessage)
		{
		}
		else if (event.type == MotionNotify)
		{
			auto [px, py] = WM::Output::Get::Position(windows[event.xmotion.window].id);
			Awning::WM::Manager::Handle::Input::Mouse::Moved(px + event.xmotion.x, py + event.xmotion.y);			
		}
		else if (event.type == ButtonPress)
		{
			if (event.xbutton.button > Button3)
			{
				if (event.xbutton.button == Button4)
				{
					Awning::WM::Manager::Handle::Input::Mouse::Scroll(0, false, 15);
				}
				if (event.xbutton.button == Button5)
				{
					Awning::WM::Manager::Handle::Input::Mouse::Scroll(0, true, 15);
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