#include "gles2.hpp"
#include "log.hpp"

#include "backends/manager.hpp"

#include "wm/manager.hpp"
#include "wm/x/wm.hpp"

#include "protocols/wl/pointer.hpp"
#include "protocols/zwp/dmabuf.hpp"

#include <string.h>

#include <fmt/format.h>
#include <libdrm/drm_fourcc.h>

#include <iostream>
#include <sstream>
#include <algorithm>

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
			wl_protocol_logger* logger; 
			wl_listener client_listener;
		};
		extern Data data;
	}
};


int LoadOpenGLES2();

static const char* vertexShaderCode = R"(
#version 300 es
precision mediump float;

out vec4 color;

void main()
{
	float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);
	float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);

	gl_Position = vec4(-1.0+x*2.0,-1.0+y*2.0,0.0,1.0);
	color       = vec4(     x    ,     y    ,0.0,1.0);
}
)";

static const char* pixelShaderCode2D = R"(
#version 300 es

precision mediump float;

uniform sampler2D texture0;

in vec4 color;
out vec4 FragColor;

void main()
{
	FragColor = texture2D(texture0, color.xy);
}
)";

static const char* pixelShaderCodeOES = R"(
#version 300 es
#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform samplerExternalOES texture0;

in vec4 color;
out vec4 FragColor;

void main()
{
	FragColor = texture2D(texture0, color.xy).bgra;
}
)";

namespace Awning::Renderers::GLES2
{
	GLuint FBO;
	GLuint FBO_Texture;
	GLuint programOES;
	GLuint program2D;
	uint8_t* buffer = nullptr;
	int bitsPerPixel;
	int bytesPerLine;
	int width  = 0;
	int height = 0;
	int size;

	GLuint frameTexture;

	void RenderWindow(WM::Window* window, int count = 2, int frame = 1, int depth = 0)
	{
		if (window->DrawingManaged() && depth == 0)
			return;

		auto texture = window->Texture();

		auto winPosX  = window->XPos   ();
		auto winPosY  = window->YPos   ();
		auto winSizeX = window->XSize  ();
		auto winSizeY = window->YSize  ();
		auto winOffX  = window->XOffset();
		auto winOffY  = window->YOffset();

		int posX  = winPosX  - winOffX                     ;
		int posY  = winPosY  - winOffY                     ;
		int sizeX = winSizeX + winOffX * std::max(count, 0);
		int sizeY = winSizeY + winOffY * std::max(count, 0);

		int frameSX = Frame::Size::left   * frame;
		int frameSY = Frame::Size::top    * frame;
		int frameEX = Frame::Size::right  * frame;
		int frameEY = Frame::Size::bottom * frame;

		if (window->Frame())
		{
			//glViewport(posX - frameSX, posY - frameSY, sizeX + frameSX + frameEX, sizeY + frameSY + frameEY);

			glUseProgram(program2D);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frameTexture);

			glViewport(posX - frameSX, posY - frameSY, sizeX + frameSX + frameEX, frameSY);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glViewport(posX - frameSX, posY + sizeY, sizeX + frameSX + frameEX, frameEY);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glViewport(posX - frameSX, posY - frameSY, frameSX, sizeY + frameSY + frameEY);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glViewport(posX + sizeX, posY - frameSY, frameEX, sizeY + frameSY + frameEY);
			glDrawArrays(GL_TRIANGLES, 0, 6);

		}

		if (!window->Mapped())
			return;
		if (!texture)
			return;

		glViewport(posX, posY, sizeX, sizeY);

		if (texture->buffer.offscreen)
		{
			glUseProgram(programOES);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture->buffer.number);
		}
		else
		{
			glUseProgram(program2D);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->buffer.number);
		}

		glDrawArrays(GL_TRIANGLES, 0, 6);

		auto subwindows = window->GetSubwindows();
		for (auto& subwindow : reverse(subwindows))
			RenderWindow(subwindow, 0, 0, depth + 1);
	}

	void Init()
	{
		EGL::Init();
		eglBindWaylandDisplayWL(EGL::display, Awning::Server::data.display);

		GLuint vertexShader, pixelShaderOES, pixelShader2D;

		EGL::CreateShader(vertexShader  , GL_VERTEX_SHADER  , vertexShaderCode  );
		EGL::CreateShader(pixelShaderOES, GL_FRAGMENT_SHADER, pixelShaderCodeOES);
		EGL::CreateShader(pixelShader2D , GL_FRAGMENT_SHADER, pixelShaderCode2D );

		EGL::CreateProgram(programOES, vertexShader, pixelShaderOES);
		EGL::CreateProgram(program2D , vertexShader, pixelShader2D );

		glDeleteShader(vertexShader  );
		glDeleteShader(pixelShaderOES);
		glDeleteShader(pixelShader2D );

		auto list     = WM::Manager::Window::Get();
		auto displays = Backend::GetDisplays();
		auto [nW, nH] = Backend::Size(displays);

		width  = nW;
		height = nH;

		bitsPerPixel = 32;
		bytesPerLine = width * 4;
		size = width * 4 * height;

		buffer = new uint8_t[size];

		glGenTextures(1, &FBO_Texture);
		glBindTexture(GL_TEXTURE_2D, FBO_Texture);

		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBO_Texture, 0);

		glGenTextures(1, &frameTexture);
		glBindTexture(GL_TEXTURE_2D, frameTexture);

		GLubyte frameColor[] = { 0xFF, 0xFF, 0x00, 0xFF };

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameColor);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void Draw()
	{
		auto list     = WM::Manager::Window::Get();
		auto displays = Backend::GetDisplays();
		auto [nW, nH] = Backend::Size(displays);

		if (nW != width || nH != height)
		{
			delete buffer;

			width  = nW;
			height = nH;

			bitsPerPixel = 32;
			bytesPerLine = width * 4;
			size = width * 4 * height;

			buffer = new uint8_t[size];	
		}

		eglMakeCurrent(EGL::display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL::context);
		
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		glBindTexture(GL_TEXTURE_2D, FBO_Texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glViewport(0, 0, width, height);
		glClearColor(238 / 255., 238 / 255., 238 / 255., 1);
		glClear(GL_COLOR_BUFFER_BIT);

		for (auto& window : reverse(list))
			RenderWindow(window);

		if (Awning::Protocols::WL::Pointer::data.window)
			RenderWindow(Awning::Protocols::WL::Pointer::data.window, 0, 0);
	
		for (auto& display : displays)
		{
			auto [px, py] = WM::Output::Get::Position(display.output);

			int sx = display.texture.bytesPerLine / (display.texture.bitsPerPixel / 8);
			int sy = display.texture.height;

			glReadPixels(px, py, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, display.texture.buffer.pointer);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	namespace FillTextureFrom
	{
		void EGLImage(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(EGL::display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL::context);

			EGLint width, height;
			eglQueryWaylandBufferWL(EGL::display, buffer, EGL_WIDTH , &width );
			eglQueryWaylandBufferWL(EGL::display, buffer, EGL_HEIGHT, &height);

			texture->width            = width ;
			texture->height           = height;
			texture->bitsPerPixel     = 32;
			texture->bytesPerLine     = width                 * (texture->bitsPerPixel / 8);
			texture->size             = texture->bytesPerLine *  texture->height           ;
			texture->red              = { .size = 8, .offset =  0 };
			texture->green            = { .size = 8, .offset =  8 };
			texture->blue             = { .size = 8, .offset = 16 };
			texture->alpha            = { .size = 8, .offset = 24 };
			texture->buffer.offscreen = true;

			EGLint attribs[] = {
				EGL_WAYLAND_PLANE_WL, 0,
				EGL_NONE 
			};
			auto image = eglCreateImageKHR(EGL::display, EGL_NO_CONTEXT, EGL_WAYLAND_BUFFER_WL, buffer, attribs);
			
			if (texture->buffer.number != 0)
				glDeleteTextures(1, &texture->buffer.number);

			glGenTextures(1, &texture->buffer.number);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture->buffer.number);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
			eglDestroyImageKHR(EGL::display, image);
		}

		void SHMBuffer(wl_shm_buffer* shm_buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(EGL::display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL::context);

			texture->width            = wl_shm_buffer_get_width (shm_buffer);
			texture->height           = wl_shm_buffer_get_height(shm_buffer);
			texture->bitsPerPixel     = 32;
			texture->bytesPerLine     = wl_shm_buffer_get_stride(shm_buffer);
			texture->size             = texture->bytesPerLine * texture->height;
			texture->red              = { .size = 8, .offset = 16 };
			texture->green            = { .size = 8, .offset =  8 };
			texture->blue             = { .size = 8, .offset =  0 };
			texture->alpha            = { .size = 8, .offset = 24 };
			texture->buffer.offscreen = false;

			auto shm_data = (uint8_t*)wl_shm_buffer_get_data(shm_buffer);

			if (texture->buffer.number != 0)
				glDeleteTextures(1, &texture->buffer.number);
			
			glGenTextures(1, &texture->buffer.number);
			glBindTexture(GL_TEXTURE_2D, texture->buffer.number);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, shm_data);

    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void LinuxDMABuf(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(EGL::display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL::context);

			auto attributes = (Protocols::ZWP::Linux_Buffer_Params::Data::Instance*)wl_resource_get_user_data(buffer);

			bool hasMod = false;
			if (attributes->modifier != DRM_FORMAT_MOD_INVALID && attributes->modifier != DRM_FORMAT_MOD_LINEAR) {
				hasMod = true;
			}

			texture->width            = attributes->width ;
			texture->height           = attributes->height;
			texture->bitsPerPixel     = 32;
			texture->bytesPerLine     = texture->width        * (texture->bitsPerPixel / 8);
			texture->size             = texture->bytesPerLine *  texture->height           ;
			texture->red              = { .size = 8, .offset =  0 };
			texture->green            = { .size = 8, .offset =  8 };
			texture->blue             = { .size = 8, .offset = 16 };
			texture->alpha            = { .size = 8, .offset = 24 };
			texture->buffer.offscreen = true;

			struct {
				EGLint fd;
				EGLint offset;
				EGLint pitch;
				EGLint mod_lo;
				EGLint mod_hi;
			} attr_names[Protocols::ZWP::Linux_Buffer_Params::DMABUF_MAX_PLANES] = {
				{
					EGL_DMA_BUF_PLANE0_FD_EXT,
					EGL_DMA_BUF_PLANE0_OFFSET_EXT,
					EGL_DMA_BUF_PLANE0_PITCH_EXT,
					EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
					EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
				}, {
					EGL_DMA_BUF_PLANE1_FD_EXT,
					EGL_DMA_BUF_PLANE1_OFFSET_EXT,
					EGL_DMA_BUF_PLANE1_PITCH_EXT,
					EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
					EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT
				}, {
					EGL_DMA_BUF_PLANE2_FD_EXT,
					EGL_DMA_BUF_PLANE2_OFFSET_EXT,
					EGL_DMA_BUF_PLANE2_PITCH_EXT,
					EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
					EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT
				}, {
					EGL_DMA_BUF_PLANE3_FD_EXT,
					EGL_DMA_BUF_PLANE3_OFFSET_EXT,
					EGL_DMA_BUF_PLANE3_PITCH_EXT,
					EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
					EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT
				}
			};
			
			unsigned int atti = 0;
			EGLint attribs[50];
			attribs[atti++] = EGL_WIDTH;
			attribs[atti++] = attributes->width;
			attribs[atti++] = EGL_HEIGHT;
			attribs[atti++] = attributes->height;
			attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
			attribs[atti++] = attributes->format;

			for (int i=0; i < attributes->planesUsed; i++) {
				attribs[atti++] = attr_names[i].fd;
				attribs[atti++] = attributes->planes[i].fd;
				attribs[atti++] = attr_names[i].offset;
				attribs[atti++] = attributes->planes[i].offset;
				attribs[atti++] = attr_names[i].pitch;
				attribs[atti++] = attributes->planes[i].stride;
				if (hasMod)
				{
					attribs[atti++] = attr_names[i].mod_lo;
					attribs[atti++] = attributes->modifier & 0xFFFFFFFF;
					attribs[atti++] = attr_names[i].mod_hi;
					attribs[atti++] = attributes->modifier >> 32;
				}
			}
			attribs[atti++] = EGL_NONE;

			auto image = eglCreateImageKHR(EGL::display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
			
			if (texture->buffer.number != 0)
				glDeleteTextures(1, &texture->buffer.number);

			glGenTextures(1, &texture->buffer.number);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture->buffer.number);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
			eglDestroyImageKHR(EGL::display, image);
		}
	}
}


