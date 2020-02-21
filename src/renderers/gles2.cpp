#include "gles2.hpp"
#include "log.hpp"

#include "backends/manager.hpp"

#include "wm/manager.hpp"

#include "protocols/wl/pointer.hpp"

#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gbm.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <fmt/format.h>

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

#define GL_TEXTURE_EXTERNAL_OES 0x8D65

void CreateShader(GLuint& shader, GLenum type, const char* code, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());
void CreateProgram(GLuint& program, GLuint vertexShader, GLuint pixelShader, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());

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

		glViewport(winPosX - winOffX, winPosY - winOffY, winSizeX, winSizeY);

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
	}

	void Init()
	{
		LoadOpenGLES2();
		eglBindWaylandDisplayWL(Awning::Server::data.egl.display, Awning::Server::data.display);

		GLuint vertexShader, pixelShaderOES, pixelShader2D;

		CreateShader(vertexShader  , GL_VERTEX_SHADER  , vertexShaderCode  );
		CreateShader(pixelShaderOES, GL_FRAGMENT_SHADER, pixelShaderCodeOES);
		CreateShader(pixelShader2D , GL_FRAGMENT_SHADER, pixelShaderCode2D );

		CreateProgram(programOES, vertexShader, pixelShaderOES);
		CreateProgram(program2D , vertexShader, pixelShader2D );

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

		eglMakeCurrent(Awning::Server::data.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, Awning::Server::data.egl.context);
		
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
			auto [px, py] = WM::Output::Get::      Position  (display.output              );
			auto [sx, sy] = WM::Output::Get::Mode::Resolution(display.output, display.mode);

			glReadPixels(px, py, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, display.texture.buffer.pointer);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	namespace FillTextureFrom
	{
		void EGLImage(wl_resource* buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(Awning::Server::data.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, Awning::Server::data.egl.context);

			EGLint width, height;
			eglQueryWaylandBufferWL(Server::data.egl.display, buffer, EGL_WIDTH , &width );
			eglQueryWaylandBufferWL(Server::data.egl.display, buffer, EGL_HEIGHT, &height);

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
			auto image = eglCreateImageKHR(Server::data.egl.display, EGL_NO_CONTEXT, EGL_WAYLAND_BUFFER_WL, buffer, attribs);
			
			if (texture->buffer.number != 0)
				glDeleteTextures(1, &texture->buffer.number);

			glGenTextures(1, &texture->buffer.number);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture->buffer.number);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

			glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
			eglDestroyImageKHR(Server::data.egl.display, image);
		}

		void SHMBuffer(wl_shm_buffer* shm_buffer, WM::Texture* texture, WM::Damage damage)
		{
			eglMakeCurrent(Awning::Server::data.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, Awning::Server::data.egl.context);

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
	}
}

static void eglLog(EGLenum error, const char *command, EGLint msg_type, EGLLabelKHR thread, EGLLabelKHR obj, const char *msg) {
	std::cout << fmt::format("[EGL] command: {}, error: 0x{:X}, message: \"{}\"\n", command, error, msg);
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

int LoadOpenGLES2()
{
	loadEGLProc(&eglGetPlatformDisplayEXT, "eglGetPlatformDisplayEXT");

	/*
	for(auto& di : std::filesystem::directory_iterator("/dev/dri"))
	{
		auto p = di.path();
		std::string s = p.filename();

		if (s.starts_with("renderD"))
		{
			drm_version version = {0};
			drm_irq_busid irq = {0};
			int dri_fd = open(p.c_str(), O_RDWR | O_CLOEXEC);
			ioctl(dri_fd, DRM_IOCTL_VERSION, &version);
			ioctl(dri_fd, DRM_IOCTL_IRQ_BUSID, &irq);

			version.name = (char*)malloc(version.name_len);
			version.date = (char*)malloc(version.date_len);
			version.desc = (char*)malloc(version.desc_len);

			ioctl(dri_fd, DRM_IOCTL_VERSION, &version);
			close(dri_fd);

			std::cout << s << " | Name: " << version.name << " | Date: " << version.date << " | Desc: " << version.desc
					  <<      " | Bus : " << irq.busnum   << " | Dev : " << irq.devnum   <<
			"\n";

			free(version.name);
			free(version.date);
			free(version.desc);
		}
	}
	*/

	int32_t fd = open("/dev/dri/renderD128", O_RDWR);
	struct gbm_device* gbm = gbm_create_device(fd);

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

	auto error = glGetError();

	if (error != 0 && error != 1280)
		return error;

	std::cout << "EGL Vendor    : " << eglQueryString(Awning::Server::data.egl.display, EGL_VENDOR ) << "\n";
	std::cout << "EGL Version   : " << eglQueryString(Awning::Server::data.egl.display, EGL_VERSION) << "\n";
	std::cout << "GL Vendor     : " << glGetString(GL_VENDOR                  ) << "\n";
	std::cout << "GL Renderer   : " << glGetString(GL_RENDERER                ) << "\n";
	std::cout << "GL Version    : " << glGetString(GL_VERSION                 ) << "\n";
	std::cout << "GLSL Version  : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

	return error;
}

void CreateShader(GLuint& shader, GLenum type, const char* code, std::experimental::fundamentals_v2::source_location function)
{
	int  success;
	char infoLog[512];

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
	    glGetShaderInfoLog(shader, 512, NULL, infoLog);

	    Log::Report::Error(fmt::format("Shader Compilation Failed {}", infoLog), function);
	}
}	

void CreateProgram(GLuint& program, GLuint vertexShader, GLuint pixelShader, std::experimental::fundamentals_v2::source_location function)
{	
	int  success;
	char infoLog[512];

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, pixelShader);
	glLinkProgram(program);

	glGetShaderiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
	    glGetShaderInfoLog(program, 512, NULL, infoLog);

	    Log::Report::Error(fmt::format("Program Linking Failed {}", infoLog), function);
	}
}