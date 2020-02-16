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

static Display*		             display;
static Window		             window;
static GLXContext	             context;
static GLuint		             texture;
static EGLDisplay                egl_display;
static EGLContext                egl_context;
static EGLSurface                egl_surface;
             
static bool 		             resized = false;
static Awning::WM::Texture framebuffer;

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
	FragColor = texture2D(texture0, color.xy);
}
)";

namespace Awning::Backend::X11
{
	void CreateBackingTexture()
	{
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer.width, framebuffer.height, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		framebuffer.buffer.pointer = new uint8_t[framebuffer.size];
	}

	void ReleaseBackingTexture()
	{
		glDeleteTextures(1, &texture);

		if (framebuffer.buffer.pointer)
		{
			delete framebuffer.buffer.pointer;
			framebuffer.buffer.pointer = nullptr;
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

	attribs.event_mask = StructureNotifyMask|ButtonPressMask|KeyPressMask|PointerMotionMask|ButtonReleaseMask|KeyReleaseMask;
	
	window = XCreateWindow(display, root, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWColormap | CWEventMask, &attribs);
	XStoreName(display, window, "Awning (X11 Backend)");
	XMapWindow(display, window);
	
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
	egl_surface = eglCreateWindowSurface(egl_display, ecfg, window, NULL);
	egl_context = eglCreateContext(egl_display, ecfg, EGL_NO_CONTEXT, ctxattr);
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

	std::cout << "X EGL Vendor  : " << eglQueryString(egl_display, EGL_VENDOR ) << "\n";
	std::cout << "X EGL Version : " << eglQueryString(egl_display, EGL_VERSION) << "\n";
	std::cout << "X GL Vendor   : " << glGetString(GL_VENDOR                  ) << "\n";
	std::cout << "X GL Renderer : " << glGetString(GL_RENDERER                ) << "\n";
	std::cout << "X GL Version  : " << glGetString(GL_VERSION                 ) << "\n";
	std::cout << "X GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

	framebuffer = Awning::WM::Texture {
        .size         = width * height * 4,
        .bitsPerPixel = 32,
        .bytesPerLine = width * 4,
        .red          = { 
			.size   = 8,
			.offset = 0
		},
        .green        = { 
			.size   = 8,
			.offset = 8
		},
        .blue         = { 
			.size   = 8,
			.offset = 16
		},
        .alpha        = { 
			.size   = 8,
			.offset = 24
		},
        .width        = width,
        .height       = height
    };

	CreateBackingTexture();
	glClearColor(1, 1, 1, 1);
	CreateShader(vertexShader, GL_VERTEX_SHADER, vertexShaderCode);
	CreateShader(pixelShader, GL_FRAGMENT_SHADER, pixelShaderCode);
	CreateProgram(program, vertexShader, pixelShader);
	glUseProgram(program);

	Output output {
		.manufacturer = "X.Org Foundation",
		.model        = "11.0",
		.physical     = {
			.width  = 0,
			.height = 0,
		},
		.modes        = {
			Output::Mode {
				.resolution   = {
					.width  = framebuffer.width,
					.height = framebuffer.height,
				},
				.refresh_rate = 60000,
				.prefered     = true,
				.current      = true,
			}
		}
	};
	Outputs::Add(output);
}

uint32_t XorgMouseToLinuxInputMouse(uint32_t button)
{
	if (button == Button1) return BTN_LEFT;
	if (button == Button2) return BTN_MIDDLE;
	if (button == Button3) return BTN_RIGHT;
	return BTN_EXTRA;
}

uint32_t XorgKeyboardToLinuxInputKeyboard(uint32_t button)
{
}

void Awning::Backend::X11::Hand()
{
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

	XEvent event;
	int preWidth = framebuffer.width, preHeight = framebuffer.height;

	while(XPending(display))
	{
    	XNextEvent(display, &event);

		if (event.type == ConfigureNotify)
		{
			framebuffer.width   = event.xconfigurerequest.width ;
			framebuffer.height  = event.xconfigurerequest.height;

			if (preWidth != framebuffer.width || preHeight != framebuffer.height)
				resized = true;
		}
		else if (event.type == ClientMessage)
		{
		}
		else if (event.type == MotionNotify)
		{
			Awning::WM::Manager::Handle::Input::Mouse::Moved(event.xbutton.x, event.xbutton.y);			
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

	if (resized)
	{
		framebuffer.size = framebuffer.width * framebuffer.height * 4;
		framebuffer.bytesPerLine = framebuffer.width * 4;

		ReleaseBackingTexture();
		CreateBackingTexture();

		Output output {
			.manufacturer = "X.Org Foundation",
			.model        = "11.0",
			.physical     = {
				.width  = 0,
				.height = 0,
			},
			.modes        = {
				Output::Mode {
					.resolution   = {
						.width  = framebuffer.width,
						.height = framebuffer.height,
					},
					.refresh_rate = 60000,
					.prefered     = true,
					.current      = true,
				}
			}
		};
		Outputs::Update(0, output);

		resized = false;
	}

	memset(framebuffer.buffer.pointer, 0xEE, framebuffer.size);
}

void Awning::Backend::X11::Draw()
{
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, framebuffer.width, framebuffer.height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer.width, framebuffer.height, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.buffer.pointer);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	eglSwapBuffers(egl_display, egl_surface);
}

void Awning::Backend::X11::Poll()
{
}

Awning::WM::Texture Awning::Backend::X11::Data()
{
	return framebuffer;
}