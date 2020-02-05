#include <GL/gl.h>
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

#include "X11.hpp"
#include "manager.hpp"

static Display*		             display;
static Window		             window;
static GLXContext	             context;
static GLuint		             texture;
             
static bool 		             resized = false;
static Awning::WM::Texture::Data framebuffer;

static int doubleBufferAttributes[] = {
    GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE,      8,     /* the maximum number of bits per component    */
    GLX_GREEN_SIZE,    8, 
    GLX_BLUE_SIZE,     8,
	GLX_ALPHA_SIZE,    8,
    None
};

using namespace std::chrono_literals;

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

		framebuffer.buffer.u8 = new uint8_t[framebuffer.size];
	}

	void ReleaseBackingTexture()
	{
		glDeleteTextures(1, &texture);

		if (framebuffer.buffer.u8)
		{
			delete framebuffer.buffer.u8;
			framebuffer.buffer.u8 = nullptr;
		}
	}
}

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

	auto vi = glXChooseVisual(display, screen, doubleBufferAttributes);
	attribs.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
	attribs.event_mask = StructureNotifyMask|ButtonPressMask|KeyPressMask|PointerMotionMask|ButtonReleaseMask|KeyReleaseMask;
	
	window = XCreateWindow(display, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &attribs);
	XStoreName(display, window, "Awning (X11 Backend)");
	XMapWindow(display, window);

	context = glXCreateContext(display, vi, NULL, true);
	glXMakeCurrent(display, window, context);

	fprintf(stdout, "GL Version    : %s\n", glGetString(GL_VERSION ));
	fprintf(stdout, "GL Vendor     : %s\n", glGetString(GL_VENDOR  ));
	fprintf(stdout, "GL Renderer   : %s\n", glGetString(GL_RENDERER));

	framebuffer = Awning::WM::Texture::Data {
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

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, framebuffer.width, framebuffer.height);

	memset(framebuffer.buffer.u8, 0, framebuffer.size);
}

void Awning::Backend::X11::Draw()
{
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer.width, framebuffer.height, 
		GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.buffer.u8);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex3f(-1.0f, -1.0f, 0.0f);
		glTexCoord2f(0.0, 0.0); glVertex3f(-1.0f,  1.0f, 0.0f);
		glTexCoord2f(1.0, 0.0); glVertex3f( 1.0f,  1.0f, 0.0f);
		glTexCoord2f(1.0, 1.0); glVertex3f( 1.0f, -1.0f, 0.0f);
	glEnd();
	glXSwapBuffers(display, window);
}

void Awning::Backend::X11::Poll()
{
}

Awning::WM::Texture::Data Awning::Backend::X11::Data()
{
	return framebuffer;
}