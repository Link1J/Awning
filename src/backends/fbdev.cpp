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

static Awning::WM::Texture::Data framebuffer;
static uint8_t* framebufferMaped;
static int fd, tty_fd;

void Awning::Backend::FBDEV::Start()
{
	//int tty_fd = open("/dev/tty0", O_RDWR);
	//ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);
	//ioctl(tty_fd, KDSETMODE, KD_TEXT);

	fd = open("/dev/fb0", O_RDWR);

	fb_fix_screeninfo finf;
	fb_var_screeninfo vinf;

	ioctl(fd, FBIOGET_FSCREENINFO, &finf);
	ioctl(fd, FBIOGET_VSCREENINFO, &vinf);

	framebuffer = Awning::WM::Texture::Data {
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
        .width        = vinf.xres,
        .height       = vinf.yres
    };

	framebuffer.buffer.u8 = new uint8_t[framebuffer.size];
	framebufferMaped = (uint8_t*)mmap(0, framebuffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);

	Output output {
		.manufacturer = finf.id,
		.model        = "N/A",
		.physical     = {
			.width  = vinf.width,
			.height = vinf.height,
		},
		.modes        = {
			Output::Mode {
				.resolution   = {
					.width  = vinf.xres,
					.height = vinf.yres,
				},
				.refresh_rate = 0,
				.prefered     = true,
				.current      = true,
			}
		}
	};
	Outputs::Add(output);
}

void Awning::Backend::FBDEV::Poll()
{
	memset(framebuffer.buffer.u8, 0, framebuffer.size);
}

void Awning::Backend::FBDEV::Draw()
{
	memcpy(framebufferMaped, framebuffer.buffer.u8, framebuffer.size);
}

Awning::WM::Texture::Data Awning::Backend::FBDEV::Data()
{
	return framebuffer;
}