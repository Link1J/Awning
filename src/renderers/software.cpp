#include "software.hpp"
#include <spdlog/spdlog.h>

#include "backends/manager.hpp"

#include "wm/manager.hpp"

#include "protocols/wl/pointer.hpp"

#include "protocols/zwp/dmabuf.hpp"
#include <libdrm/drm_fourcc.h>

#include <string.h>

#include "egl.hpp"

#include <iostream>

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

static const char* pixelShaderCode = R"(
#version 300 es
#extension GL_OES_EGL_image_external : require

precision mediump float;

uniform samplerExternalOES texture0;

in vec4 color;
out vec4 FragColor;

void main()
{
	FragColor = texture2D(texture0, color.xy);
}
)";

namespace Awning::Renderers::Software
{
	uint8_t* buffer = nullptr;
	int bitsPerPixel;
	int bytesPerLine;
	int width  = 0;
	int height = 0;
	int size;

	void RenderWindow(WM::Window* window, int count = 2, int frame = 1, int depth = 0)
	{
		auto texture = window->Texture();

		if (!window->Mapped())
			return;
		if (!texture)
			return;

		auto winPosX  = window->XPos   ();
		auto winPosY  = window->YPos   ();
		auto winSizeX = window->XSize  ();
		auto winSizeY = window->YSize  ();
		auto winOffX  = window->XOffset();
		auto winOffY  = window->YOffset();

		int posX    = winPosX ;
		int posY    = winPosY ;
		int sizeX   = winSizeX;
		int sizeY   = winSizeY;
		int offX    = winOffX ;
		int offY    = winOffY ;

		int frameSX = Frame::Size::left   * frame;
		int frameSY = Frame::Size::top    * frame;
		int frameEX = Frame::Size::right  * frame;
		int frameEY = Frame::Size::bottom * frame;

		for (int x = -frameSX; x < sizeX + frameEX; x++)
			for (int y = -frameSY; y < sizeY + frameEY; y++)
			{
				if ((posX + x) <  0     )
					continue;
				if ((posY + y) <  0     )
					continue;
				if ((posX + x) >= width )
					continue;
				if ((posY + y) >= height)
					continue;

				int windowOffset = (x + offX - 1) * (texture->bitsPerPixel / 8)
								 + (y + offY - 1) *  texture->bytesPerLine    ;

				int framebOffset = (posX + x) * (bitsPerPixel / 8)
								 + (posY + y) *  bytesPerLine    ;

				uint8_t red, green, blue, alpha;

				if (x < texture->width && y < texture->height && x >= 0 && y >= 0)
				{
					if (texture->buffer.pointer != nullptr && windowOffset < texture->size)
					{
						red   = texture->buffer.pointer[windowOffset + (texture->red  .offset / 8)];
						green = texture->buffer.pointer[windowOffset + (texture->green.offset / 8)];
						blue  = texture->buffer.pointer[windowOffset + (texture->blue .offset / 8)];
						alpha = texture->buffer.pointer[windowOffset + (texture->alpha.offset / 8)];
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

				if (alpha > 0 && framebOffset < size)
				{
					uint8_t& buffer_red   = buffer[framebOffset + 2];
					uint8_t& buffer_green = buffer[framebOffset + 1];
					uint8_t& buffer_blue  = buffer[framebOffset + 0];

					buffer_red   = red  ; // * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
					buffer_green = green; // * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
					buffer_blue  = blue ; // * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
				}
			}
		
		auto subwindows = window->GetSubwindows();
		for (auto& subwindow : reverse(subwindows))
			RenderWindow(subwindow, 0, 0, depth + 1);
	}

	GLuint vertexShader, pixelShader, program;

	void Init()
	{
		if (EGL::Init() != 0)
			return;
		
		eglBindWaylandDisplayWL(EGL::display, Awning::Server::data.display);

		EGL::CreateShader(vertexShader, GL_VERTEX_SHADER, vertexShaderCode);
		EGL::CreateShader(pixelShader, GL_FRAGMENT_SHADER, pixelShaderCode);
		EGL::CreateProgram(program, vertexShader, pixelShader);
		glUseProgram(program);
	}

	void Draw()
	{
		auto displays = Backend::GetDisplays();
		auto [nW, nH] = Backend::Size(displays);

		if (nW != width || nH != height)
		{
			if (buffer)
				delete buffer;

			width  = nW;
			height = nH;

			bitsPerPixel = 32;
			bytesPerLine = width * 4;
			size = width * 4 * height;

			buffer = new uint8_t[size];	
		}

		memset(buffer, 0xEE, size);

		int layerOrder[] = {
			(int)WM::Window::Manager::Layer::Background,
			(int)WM::Window::Manager::Layer::Bottom,
			(int)WM::Window::Manager::Layer::Application,
			(int)WM::Window::Manager::Layer::Top,
			(int)WM::Window::Manager::Layer::Overlay,
		};

		for (int a = 0; a < sizeof(layerOrder) / sizeof(*layerOrder); a++)
		{
			auto list = WM::Window::Manager::layers[layerOrder[a]];
			for (auto& window : reverse(list))
			{
				RenderWindow(window);
			}
		}

		if (Awning::Protocols::WL::Pointer::data.window)
		{
			auto texture = Awning::Protocols::WL::Pointer::data.window->Texture();

			if (texture)
			{
				RenderWindow(Awning::Protocols::WL::Pointer::data.window, 0, 0);
			}
			else
			{
				auto winPosX  = Awning::Protocols::WL::Pointer::data.window->XPos();
				auto winPosY  = Awning::Protocols::WL::Pointer::data.window->YPos();

				for (int x = 0; x < 5; x++)
					for (int y = 0; y < 5; y++)
					{
						if ((winPosX + x) <  0     )
							continue;
						if ((winPosY + y) <  0     )
							continue;
						if ((winPosX + x) >= width )
							continue;
						if ((winPosY + y) >= height)
							continue;

						int framebOffset = (winPosX + x) * (bitsPerPixel / 8)
										 + (winPosY + y) *  bytesPerLine    ;

						buffer[framebOffset + 2] = 0x00;
						buffer[framebOffset + 1] = 0xFF;
						buffer[framebOffset + 0] = 0x00;
					}
			}			
		}

		for (auto& display : displays)
		{
			auto [px, py] = WM::Output::Get::      Position  (display.output              );
			auto [sx, sy] = WM::Output::Get::Mode::Resolution(display.output, display.mode);

			for (int x = 0; x < sx; x++)
				for (int y = 0; y < sy; y++)
				{
					if ((px + x) <  0     )
						continue;
					if ((py + y) <  0     )
						continue;
					if ((px + x) >= width )
						continue;
					if ((py + y) >= height)
						continue;

					int framebOffset = (px + x) * (bitsPerPixel / 8)
									 + (py + y) *  bytesPerLine    ;
					int windowOffset = (x) * (display.texture.bitsPerPixel / 8)
									 + (y) *  display.texture.bytesPerLine    ;

					display.texture.buffer.pointer[windowOffset + (display.texture.red  .offset / 8)] = buffer[framebOffset + 2];
					display.texture.buffer.pointer[windowOffset + (display.texture.green.offset / 8)] = buffer[framebOffset + 1];
					display.texture.buffer.pointer[windowOffset + (display.texture.blue .offset / 8)] = buffer[framebOffset + 0];
				}
		}
	}

	namespace FillTextureFrom
	{
		void SizeTextureBuffer(WM::Texture* texture, int pitch, int height)
		{
			auto size = pitch * height;
			if (texture->size != size)
			{
				if (texture->buffer.pointer)
					delete texture->buffer.pointer;
				texture->buffer.pointer = (uint8_t*)malloc(size);
			}
		}

		void EGLImage(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(EGL::display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL::context);
		
			EGLint width, height;
			eglQueryWaylandBufferWL(EGL::display, buffer, EGL_WIDTH , &width );
			eglQueryWaylandBufferWL(EGL::display, buffer, EGL_HEIGHT, &height);

			SizeTextureBuffer(texture, width * 4, height);

			texture->width        = width ;
			texture->height       = height;
			texture->bitsPerPixel = 32;
			texture->bytesPerLine = width                 * (texture->bitsPerPixel / 8);
			texture->size         = texture->bytesPerLine *  texture->height           ;
			texture->red          = { .size = 8, .offset =  0 };
			texture->green        = { .size = 8, .offset =  8 };
			texture->blue         = { .size = 8, .offset = 16 };
			texture->alpha        = { .size = 8, .offset = 24 };

			EGLint attribs[] = {
				EGL_WAYLAND_PLANE_WL, 0,
				EGL_NONE 
			};
			auto image = eglCreateImageKHR(EGL::display, EGL_NO_CONTEXT, EGL_WAYLAND_BUFFER_WL, buffer, attribs);
			
			GLuint textures[2];
			GLuint fbo;

			glGenTextures(2, textures);
			glGenFramebuffers(1, &fbo); 
			
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);
			glBindTexture(GL_TEXTURE_2D, textures[1]);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);

			glViewport(0, 0, texture->width, texture->height);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glReadPixels(0, 0, texture->width, texture->height, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer.pointer);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

			glDeleteFramebuffers(1, &fbo);
			glDeleteTextures(2, textures);
			eglDestroyImageKHR(EGL::display, image);
		}

		void SHMBuffer(wl_shm_buffer* shm_buffer, WM::Texture* texture, WM::Damage damage)
		{
			SizeTextureBuffer(texture, wl_shm_buffer_get_stride(shm_buffer), wl_shm_buffer_get_height(shm_buffer));

			texture->width        = wl_shm_buffer_get_width (shm_buffer);
			texture->height       = wl_shm_buffer_get_height(shm_buffer);
			texture->bitsPerPixel = 32;
			texture->bytesPerLine = wl_shm_buffer_get_stride(shm_buffer);
			texture->size         = texture->bytesPerLine * texture->height;
			texture->red          = { .size = 8, .offset = 16 };
			texture->green        = { .size = 8, .offset =  8 };
			texture->blue         = { .size = 8, .offset =  0 };
			texture->alpha        = { .size = 8, .offset = 24 };

			auto shm_data = (uint8_t*)wl_shm_buffer_get_data(shm_buffer);

			if (damage.xs > texture->width )
				damage.xs = texture->width ;
			if (damage.ys > texture->height)
				damage.ys = texture->height;
				
			for (int x = damage.xp; x < damage.xp + damage.xs; x++)
				for (int y = damage.yp; y < damage.yp + damage.ys; y++)
					for (int o = 0; o < texture->bitsPerPixel / 8; o++)
					{
						uint32_t offs = o + (x + y * texture->width) * texture->bitsPerPixel / 8;

						if (offs < texture->size)
							texture->buffer.pointer[offs] = shm_data[offs];
					}
		}

		void LinuxDMABuf(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(EGL::display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL::context);
		
			auto attributes = (Protocols::ZWP::Linux_Buffer_Params::Data::Instance*)wl_resource_get_user_data(buffer);

			bool hasMod = false;
			if (attributes->modifier != DRM_FORMAT_MOD_INVALID && attributes->modifier != DRM_FORMAT_MOD_LINEAR) {
				hasMod = true;
			}

			SizeTextureBuffer(texture, attributes->width * 4, attributes->height);

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
			
			GLuint textures[2];
			GLuint fbo;

			glGenTextures(2, textures);
			glGenFramebuffers(1, &fbo); 
			
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);
			glBindTexture(GL_TEXTURE_2D, textures[1]);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);

			glViewport(0, 0, texture->width, texture->height);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glReadPixels(0, 0, texture->width, texture->height, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer.pointer);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

			glDeleteFramebuffers(1, &fbo);
			glDeleteTextures(2, textures);
			eglDestroyImageKHR(EGL::display, image);
		}
	}
}