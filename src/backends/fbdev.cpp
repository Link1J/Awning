#include "wayland/pointer.hpp"
#include "wayland/keyboard.hpp"

#include "log.hpp"

#include <linux/fb.h>
#include <linux/kd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <chrono>
#include <thread>

#include "fbdev.hpp"
#include "manager.hpp"

#include "wm/output.hpp"

static Awning::WM::Texture framebuffer;
static uint8_t* framebufferMaped;
static int fd;
static Awning::WM::Output::ID id;
int tty_fd;

void Awning::Backend::FBDEV::Start()
{
	int tty_fd = open("/dev/tty0", O_RDWR);
	ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);
	ioctl(tty_fd, KDSKBMODE, K_OFF);

	fd = open("/dev/fb0", O_RDWR);

	fb_fix_screeninfo finf;
	fb_var_screeninfo vinf;

	ioctl(fd, FBIOGET_FSCREENINFO, &finf);
	ioctl(fd, FBIOGET_VSCREENINFO, &vinf);

	framebuffer = Awning::WM::Texture {
        .size         = finf.line_length * vinf.yres,
        .bitsPerPixel = vinf.bits_per_pixel,
        .bytesPerLine = finf.line_length,
        .red          = { 
			.size   = vinf.red.length,
			.offset = vinf.red.offset
		},
        .green        = { 
			.size   = vinf.green.length,
			.offset = vinf.green.offset
		},
        .blue         = { 
			.size   = vinf.blue.length,
			.offset = vinf.blue.offset
		},
        .alpha        = { 
			.size   = 0,
			.offset = 0
		},
        .width        = vinf.xres,
        .height       = vinf.yres
    };

	framebuffer.buffer.pointer = new uint8_t[framebuffer.size];
	framebufferMaped = (uint8_t*)mmap(0, framebuffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);

	id = WM::Output::Create();
	WM::Output::Set::NumberOfModes(id, 1);

	WM::Output::Set::Manufacturer(id, finf.id                );
	WM::Output::Set::Model       (id, "N/A"                  );
	WM::Output::Set::Size        (id, vinf.width, vinf.height);

	WM::Output::Set::Mode::Resolution (id, 0, vinf.xres, vinf.yres);
	WM::Output::Set::Mode::RefreshRate(id, 0, 0                   );
	WM::Output::Set::Mode::Prefered   (id, 0, true                );
	WM::Output::Set::Mode::Current    (id, 0, true                );
}

void Awning::Backend::FBDEV::Poll()
{
	memset(framebuffer.buffer.pointer, 0xEE, framebuffer.size);
}

void Awning::Backend::FBDEV::Draw()
{
	memcpy(framebufferMaped, framebuffer.buffer.pointer, framebuffer.size);
}

Awning::WM::Texture Awning::Backend::FBDEV::Data()
{
	return framebuffer;
}