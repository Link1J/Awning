#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gbm.h>

#include <spdlog/spdlog.h>

#include <experimental/source_location>

extern PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
extern PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;
extern PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
extern PFNEGLQUERYWAYLANDBUFFERWL eglQueryWaylandBufferWL;
extern PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL;
extern PFNEGLUNBINDWAYLANDDISPLAYWL eglUnbindWaylandDisplayWL;
extern PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT;
extern PFNEGLQUERYDMABUFMODIFIERSEXTPROC eglQueryDmaBufModifiersEXT;
extern PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA;
extern PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA;
extern PFNEGLDEBUGMESSAGECONTROLKHRPROC eglDebugMessageControlKHR;

extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

#define GL_TEXTURE_EXTERNAL_OES 0x8D65

namespace Awning::Renderers::EGL
{
	extern EGLDisplay display;
	extern EGLint major, minor;
	extern EGLContext context;
	extern EGLSurface surface;

	int Init();

	void loadEGLProc(void* proc_ptr, const char* name);

	void CreateShader(GLuint& shader, GLenum type, const char* code, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());
	void CreateProgram(GLuint& program, GLuint vertexShader, GLuint pixelShader, std::experimental::fundamentals_v2::source_location function = std::experimental::fundamentals_v2::source_location::current());
}