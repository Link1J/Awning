#include "egl.hpp"

#include <fmt/format.h>
#include <iostream>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

PFNEGLGETPLATFORMDISPLAYEXTPROC          eglGetPlatformDisplayEXT         ;
PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;
PFNEGLCREATEIMAGEKHRPROC                 eglCreateImageKHR                ;
PFNEGLDESTROYIMAGEKHRPROC                eglDestroyImageKHR               ;
PFNEGLQUERYWAYLANDBUFFERWL               eglQueryWaylandBufferWL          ;
PFNEGLBINDWAYLANDDISPLAYWL               eglBindWaylandDisplayWL          ;
PFNEGLUNBINDWAYLANDDISPLAYWL             eglUnbindWaylandDisplayWL        ;
PFNEGLQUERYDMABUFFORMATSEXTPROC          eglQueryDmaBufFormatsEXT         ;
PFNEGLQUERYDMABUFMODIFIERSEXTPROC        eglQueryDmaBufModifiersEXT       ;
PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC     eglExportDMABUFImageQueryMESA    ;
PFNEGLEXPORTDMABUFIMAGEMESAPROC          eglExportDMABUFImageMESA         ;
PFNEGLDEBUGMESSAGECONTROLKHRPROC         eglDebugMessageControlKHR        ;

PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

namespace Awning::Renderers::EGL
{
	EGLDisplay display;
	EGLint major, minor;
	EGLContext context;
	EGLSurface surface;

	static void eglLog(EGLenum error, const char *command, EGLint msg_type, EGLLabelKHR thread, EGLLabelKHR obj, const char *msg) {
		spdlog::error("[EGL] command: {}, error: 0x{:X}, message: \"{}\"\n", command, error, msg);
	}

	void loadEGLProc(void* proc_ptr, const char* name)
	{
		void* proc = (void*)eglGetProcAddress(name);
		if (proc == NULL) {
			spdlog::error("eglGetProcAddress({}) failed", name);
			abort();
		}
		*(void**)proc_ptr = proc;
	}

	int Init()
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

		display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, gbm, NULL);

		eglInitialize(display, &major, &minor);

		loadEGLProc(&eglBindWaylandDisplayWL      , "eglBindWaylandDisplayWL"      );
		loadEGLProc(&eglUnbindWaylandDisplayWL    , "eglUnbindWaylandDisplayWL"    );
		loadEGLProc(&eglQueryWaylandBufferWL      , "eglQueryWaylandBufferWL"      );
		loadEGLProc(&eglCreateImageKHR            , "eglCreateImageKHR"            );
		loadEGLProc(&eglDestroyImageKHR           , "eglDestroyImageKHR"           );
		loadEGLProc(&glEGLImageTargetTexture2DOES , "glEGLImageTargetTexture2DOES" );
		loadEGLProc(&eglDebugMessageControlKHR    , "eglDebugMessageControlKHR"    );
		loadEGLProc(&eglQueryDmaBufFormatsEXT     , "eglQueryDmaBufFormatsEXT"     );
		loadEGLProc(&eglQueryDmaBufModifiersEXT   , "eglQueryDmaBufModifiersEXT"   );
		loadEGLProc(&eglExportDMABUFImageQueryMESA, "eglExportDMABUFImageQueryMESA");
		loadEGLProc(&eglExportDMABUFImageMESA     , "eglExportDMABUFImageMESA"     );

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
		eglChooseConfig(display, attribs, &config, 1, &num_configs_returned);

		//eglBindAPI(EGL_OPENGL_ES_API);

		EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

		context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);

		auto error = glGetError();

		if (error != 0)
			return error;

		auto extensions = std::string(eglQueryString(display, EGL_EXTENSIONS));

		spdlog::info("EGL Vendor    : {}", eglQueryString(display, EGL_VENDOR     ));
		spdlog::info("EGL Version   : {}", eglQueryString(display, EGL_VERSION    ));
		//spdlog::info("EGL Extensions: {}", extensions                              );
		spdlog::info("GL Vendor     : {}", glGetString(GL_VENDOR                  ));
		spdlog::info("GL Renderer   : {}", glGetString(GL_RENDERER                ));
		spdlog::info("GL Version    : {}", glGetString(GL_VERSION                 ));
		spdlog::info("GLSL Version  : {}", glGetString(GL_SHADING_LANGUAGE_VERSION));

		if (extensions.find("EGL_KHR_image_base") == std::string::npos)
			 std::cerr << "EGL Context does not support \'" "EGL_KHR_image_base" "\'\n";

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

		    spdlog::error("({}:{}) Shader Compilation Failed {}", function.file_name(), function.line(), infoLog);
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

			spdlog::error("({}:{}) Program Linking Failed {}", function.file_name(), function.line(), infoLog);
		}
	}
}