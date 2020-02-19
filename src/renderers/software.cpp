#include "software.hpp"
#include "log.hpp"

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

void CreateShader(GLuint& shader, GLenum type, const char* code, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());
void CreateProgram(GLuint& program, GLuint vertexShader, GLuint pixelShader, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());

namespace Awning::Renderers::Software
{
	uint8_t* buffer = nullptr;
	int bitsPerPixel;
	int bytesPerLine;
	int width  = 0;
	int height = 0;
	int size;

	void RenderWindow(WM::Window* window, int count = 2, int frame = 1)
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

		for (int x = (-Frame::Size::left * frame); x < winSizeX + (winOffX * count) + (Frame::Size::right * frame); x++)
			for (int y = (-Frame::Size::top * frame); y < winSizeY + (winOffY * count) + (Frame::Size::bottom * frame); y++)
			{
				if ((winPosX + x - winOffX) <  0     )
					continue;
				if ((winPosY + y - winOffY) <  0     )
					continue;
				if ((winPosX + x - winOffX) >= width )
					continue;
				if ((winPosY + y - winOffY) >= height)
					continue;

				int windowOffset = (x) * (texture->bitsPerPixel / 8)
								 + (y) *  texture->bytesPerLine    ;

				int framebOffset = (winPosX + x - winOffX) * (bitsPerPixel / 8)
								 + (winPosY + y - winOffY) *  bytesPerLine    ;

				uint8_t red, green, blue, alpha;

				if (x < winSizeX + winOffX && y < winSizeY + winOffY && x >= 0 && y >= 0)
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

					buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
					buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
					buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
				}
			}
	}

	GLuint vertexShader, pixelShader, program;

	void Init()
	{
		if (LoadOpenGLES2() != 0)
			return;
		
		eglBindWaylandDisplayWL(Awning::Server::data.egl.display, Awning::Server::data.display);

		CreateShader(vertexShader, GL_VERTEX_SHADER, vertexShaderCode);
		CreateShader(pixelShader, GL_FRAGMENT_SHADER, pixelShaderCode);
		CreateProgram(program, vertexShader, pixelShader);
		glUseProgram(program);
	}

	void Draw()
	{
		auto list     = WM::Manager::Window::Get();
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

		for (auto& window : reverse(list))
			RenderWindow(window);

		if (Awning::Wayland::Pointer::data.window)
		{
			auto texture = Awning::Wayland::Pointer::data.window->Texture();

			if (texture)
			{
				RenderWindow(Awning::Wayland::Pointer::data.window, 0, 0);
			}
			else
			{
				auto winPosX  = Awning::Wayland::Pointer::data.window->XPos();
				auto winPosY  = Awning::Wayland::Pointer::data.window->YPos();

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
			eglMakeCurrent(Awning::Server::data.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, Awning::Server::data.egl.context);
		
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

			glReadPixels(0, 0, texture->width, texture->height, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer.pointer);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

			glDeleteFramebuffers(1, &fbo);
			glDeleteTextures(2, textures);
			eglDestroyImageKHR(Server::data.egl.display, image);
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
	}
}