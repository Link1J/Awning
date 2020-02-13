#include "software.hpp"

#include "backends/manager.hpp"

#include "wm/manager.hpp"

#include "wayland/pointer.hpp"

#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

//#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

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
		extern Data data;
	}
};

extern PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
extern PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;
extern PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
extern PFNEGLQUERYWAYLANDBUFFERWL eglQueryWaylandBufferWL;
extern PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL;
extern PFNEGLUNBINDWAYLANDDISPLAYWL eglUnbindWaylandDisplayWL;
extern PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC eglSwapBuffersWithDamage; // KHR or EXT
extern PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT;
extern PFNEGLQUERYDMABUFMODIFIERSEXTPROC eglQueryDmaBufModifiersEXT;
extern PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA;
extern PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA;
extern PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControlKHR;

extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

#define GL_TEXTURE_EXTERNAL_OES 0x8D65

const char* vertexShaderCode = R"(
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

const char* pixelShaderCode = R"(
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
	WM::Texture data;

	void RenderWindow(WM::Window* window)
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

		for (int x = -Frame::Size::left; x < winSizeX + winOffX + winOffX + Frame::Size::right; x++)
			for (int y = -Frame::Size::top; y < winSizeY + winOffY + winOffY + Frame::Size::bottom; y++)
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

	void Init()
	{
		auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
		auto pixelShader = glCreateShader(GL_FRAGMENT_SHADER);

		int  success;
		char infoLog[512];

		glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
		glShaderSource(pixelShader, 1, &pixelShaderCode, NULL);

		glCompileShader(vertexShader);
		glCompileShader(pixelShader);

		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
		    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog;
		}

		glGetShaderiv(pixelShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
		    glGetShaderInfoLog(pixelShader, 512, NULL, infoLog);
		    std::cout << "ERROR::SHADER::PIXEL::COMPILATION_FAILED\n" << infoLog;
		}

		auto program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, pixelShader);
		glLinkProgram(program);
		glUseProgram(program);

		glGetShaderiv(program, GL_LINK_STATUS, &success);
		if(!success)
		{
		    glGetShaderInfoLog(program, 512, NULL, infoLog);
		    std::cout << "ERROR::PROGRAM::LINK::COMPILATION_FAILED\n" << infoLog;
		}
	}

	void Draw()
	{
		data = Backend::Data();
		memset(data.buffer.u8, 0xEE, data.size);
		
		auto list = WM::Manager::Window::Get();

		for (auto& window : reverse(list))
			RenderWindow(window);

		if (Awning::Wayland::Pointer::data.window)
		{
			auto texture = Awning::Wayland::Pointer::data.window->Texture();

			if (texture)
			{
				RenderWindow(Awning::Wayland::Pointer::data.window);
			}
			//else
			{
				auto winPosX  = Awning::Wayland::Pointer::data.window->XPos();
				auto winPosY  = Awning::Wayland::Pointer::data.window->YPos();

				for (int x = 0; x < 5; x++)
					for (int y = 0; y < 5; y++)
					{
						if ((winPosX + x) <  0          )
							continue;
						if ((winPosY + y) <  0          )
							continue;
						if ((winPosX + x) >= data.width )
							continue;
						if ((winPosY + y) >= data.height)
							continue;

						int framebOffset = (winPosX + x) * (data.bitsPerPixel / 8)
										 + (winPosY + y) *  data.bytesPerLine    ;

						data.buffer.u8[framebOffset + (data.red  .offset / 8)] = 0x00;
						data.buffer.u8[framebOffset + (data.green.offset / 8)] = 0xFF;
						data.buffer.u8[framebOffset + (data.blue .offset / 8)] = 0x00;
					}
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
				if (texture->buffer.u8)
					delete texture->buffer.u8;
				texture->buffer.u8 = (uint8_t*)malloc(size);
			}
		}

		void EGLImage(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			EGLint width, height;
			eglQueryWaylandBufferWL(Server::data.egl.display, buffer, EGL_WIDTH , &width );
			eglQueryWaylandBufferWL(Server::data.egl.display, buffer, EGL_HEIGHT, &height);

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
			auto image = eglCreateImageKHR(Server::data.egl.display, EGL_NO_CONTEXT, EGL_WAYLAND_BUFFER_WL, buffer, attribs);
			
			GLuint textures[2];
			GLuint fbo;

			glGenTextures(2, textures);
			glGenFramebuffers(1, &fbo); 
			
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);
			glBindTexture(GL_TEXTURE_2D, textures[1]);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, textures[0]);

			glViewport(0, 0, width, height);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glReadPixels(0, 0, texture->width, texture->height, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer.u8);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

			glDeleteFramebuffers(1, &fbo);
			glDeleteTextures(2, textures);
			eglDestroyImageKHR(Server::data.egl.display, image);
		}

		void SHMBuffer(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			auto shm_buffer = wl_shm_buffer_get(buffer);

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

			if (damage.xs > texture->bytesPerLine)
				damage.xs = texture->bytesPerLine;
			if (damage.ys > texture->height      )
				damage.ys = texture->height      ;
				
			for (int x = damage.xp; x < damage.xp + damage.xs; x++)
				for (int y = damage.yp; y < damage.yp + damage.ys; y++)
				{
					uint32_t offs = x + y * texture->bytesPerLine;

					if (offs < texture->size)
						texture->buffer.u8[offs] = shm_data[offs];
				}
		}
	}
}