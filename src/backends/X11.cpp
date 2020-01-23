#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <linux/input.h>

#include "wayland/seat.hpp"

#include "log.hpp"

#include <cstdio>
#include <stdlib.h>

#include <chrono>
#include <thread>

#include "X11.hpp"

static Display*		display;
static Window		window;
static GLXContext	context;
static GLuint		texture;

static int 			width   = 1024;
static int 			height  = 576;
static bool 		resized = false;
static char*		buffer  = nullptr;

static int doubleBufferAttributes[] = {
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER,  True,  /* Request a double-buffered color buffer with */
    GLX_RED_SIZE,      1,     /* the maximum number of bits per component    */
    GLX_GREEN_SIZE,    1, 
    GLX_BLUE_SIZE,     1,
	GLX_DEPTH_SIZE,    24,
    None
};

using namespace std::chrono_literals;

namespace X11
{
	void CreateBackingTexture()
	{
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		buffer = new char[width * height * 4];
	}

	void ReleaseBackingTexture()
	{
		glDeleteTextures(1, &texture);
	}
}

void X11::Start()
{
	int screen;
	XSetWindowAttributes attribs;
 
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);
	auto root = RootWindow(display, screen);

	GLint gl_attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	auto vi = glXChooseVisual(display, 0, doubleBufferAttributes);
	attribs.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
	attribs.event_mask = StructureNotifyMask|ExposureMask|ButtonPressMask|KeyPressMask|PointerMotionMask|ButtonReleaseMask;
	
	window = XCreateWindow(display, root, 30, 30, width , height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &attribs);
	XStoreName(display, window, "Awning (X11 Backend)");
	XMapWindow(display, window);

	context = glXCreateContext(display, vi, NULL, true);
	glXMakeCurrent(display, window, context);

	fprintf(stdout, "GL Version    : %s\n", glGetString(GL_VERSION ));
	fprintf(stdout, "GL Vendor     : %s\n", glGetString(GL_VENDOR  ));
	fprintf(stdout, "GL Renderer   : %s\n", glGetString(GL_RENDERER));

	CreateBackingTexture();
	glClearColor(0, 0, 0, 1);
}

void X11::Poll()
{
	XEvent event;
	int preWidth = width, preHeight = height;

	while(XPending(display))
	{
    	XNextEvent(display, &event);

		if (event.type == ConfigureNotify)
		{
			width   = event.xconfigurerequest.width ;
			height  = event.xconfigurerequest.height;

			if (preWidth != width || preHeight != height)
				resized = true;
		}
		else if (event.type == ClientMessage)
		{
		}
		else if (event.type == MotionNotify)
		{
			Awning::Wayland::Pointer::Moved(event.xbutton.x, event.xbutton.y);			
		}
		else if (event.type == ButtonPress)
		{
			uint32_t button = BTN_EXTRA;
			if (event.xbutton.button == Button1) button = BTN_LEFT;
			if (event.xbutton.button == Button2) button = BTN_MIDDLE;
			if (event.xbutton.button == Button3) button = BTN_RIGHT;
			Awning::Wayland::Pointer::Button(button, true);
		}
		else if (event.type == ButtonRelease)
		{
			uint32_t button = BTN_EXTRA;
			if (event.xbutton.button == Button1) button = BTN_LEFT;
			if (event.xbutton.button == Button2) button = BTN_MIDDLE;
			if (event.xbutton.button == Button3) button = BTN_RIGHT;
			Awning::Wayland::Pointer::Button(button, false);
		}
	}

	if (resized)
	{
		ReleaseBackingTexture();
		CreateBackingTexture();
		resized = false;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex3f(-1.0f, -1.0f, 0.0f);
		glTexCoord2f(0.0, 0.0); glVertex3f(-1.0f,  1.0f, 0.0f);
		glTexCoord2f(1.0, 0.0); glVertex3f( 1.0f,  1.0f, 0.0f);
		glTexCoord2f(1.0, 1.0); glVertex3f( 1.0f, -1.0f, 0.0f);
	glEnd();
	glXSwapBuffers(display, window);

	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
		{
			int framebOffset = (x + y * width) * 4;
			buffer[framebOffset + 0] = 0;
			buffer[framebOffset + 1] = 0;
			buffer[framebOffset + 2] = 0;
			buffer[framebOffset + 3] = 1;
		}
}

char* X11::Data()
{
	return buffer;
}

int X11::Width()
{
	return width;
}

int X11::Height()
{
	return height;
}