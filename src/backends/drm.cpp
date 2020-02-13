#include "wayland/pointer.hpp"
#include "wayland/keyboard.hpp"

#include "log.hpp"

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>

#include <chrono>
#include <filesystem>
#include <iostream>

#include "drm.hpp"
#include "manager.hpp"

struct Data
{
	uint32_t* memoryMappedBuffer;
	Awning::WM::Texture texture;
};

std::vector<Data> framebuffers;

void CreateDumbBuffer(int dri_fd, drm_mode_card_res* res, drm_mode_get_connector* conn, int i)
{
	struct drm_mode_create_dumb create_dumb={0};
	struct drm_mode_map_dumb map_dumb={0};
	struct drm_mode_fb_cmd cmd_dumb={0};

	Awning::WM::Texture framebuffer;

	drm_mode_modeinfo* mode_ptr = (drm_mode_modeinfo*)conn->modes_ptr;

	create_dumb.width = mode_ptr->hdisplay;
	create_dumb.height = mode_ptr->vdisplay;
	create_dumb.bpp = 32;
	create_dumb.flags = 0;
	create_dumb.pitch = 0;
	create_dumb.size = 0;
	create_dumb.handle = 0;
	ioctl(dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);

	cmd_dumb.width=create_dumb.width;
	cmd_dumb.height=create_dumb.height;
	cmd_dumb.bpp=create_dumb.bpp;
	cmd_dumb.pitch=create_dumb.pitch;
	cmd_dumb.depth=24;
	cmd_dumb.handle=create_dumb.handle;
	ioctl(dri_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb);

	map_dumb.handle=create_dumb.handle;
	ioctl(dri_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb);

	Data data;

	data.memoryMappedBuffer   = (uint32_t*)mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, dri_fd, map_dumb.offset);

	data.texture.buffer.u8    = (uint8_t*)malloc(create_dumb.size);
	data.texture.width        = create_dumb.width ;
	data.texture.height       = create_dumb.height;
	data.texture.bytesPerLine = create_dumb.pitch ;
	data.texture.size         = create_dumb.size  ;
	data.texture.bitsPerPixel = 32                ;
	data.texture.red          = { .size = 8, .offset = 16 };
	data.texture.green        = { .size = 8, .offset =  8 };
	data.texture.blue         = { .size = 8, .offset =  0 };
	data.texture.alpha        = { .size = 0, .offset = 24 };

	framebuffers.push_back(data);

	struct drm_mode_get_encoder enc={0};
	struct drm_mode_crtc crtc={0};

	enc.encoder_id = conn->encoder_id;
	ioctl(dri_fd, DRM_IOCTL_MODE_GETENCODER, &enc);	//get encoder

	crtc.crtc_id = enc.crtc_id;
	ioctl(dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc);

	crtc.fb_id = cmd_dumb.fb_id;
	crtc.set_connectors_ptr = (&res->connector_id_ptr)[i];
	crtc.count_connectors = 1;
	crtc.mode = *mode_ptr;
	crtc.mode_valid = 1;

	ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
}

void Awning::Backend::DRM::Start()
{
	for(auto& di : std::filesystem::directory_iterator("/dev/dri"))
	{
		auto p = di.path();
		std::string s = p.filename();

		if (s.starts_with("card"))
		{
			int dri_fd = open(p.c_str(), O_RDWR | O_CLOEXEC);
			ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);

			struct drm_mode_card_res res = {0};
			ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);

			res.fb_id_ptr        = (uint64_t)malloc(res.count_fbs        * sizeof(uint32_t));
			res.crtc_id_ptr      = (uint64_t)malloc(res.count_crtcs      * sizeof(uint32_t));
			res.connector_id_ptr = (uint64_t)malloc(res.count_connectors * sizeof(uint32_t));
			res.encoder_id_ptr   = (uint64_t)malloc(res.count_encoders   * sizeof(uint32_t));

			ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);

			for (int i = 0; i < res.count_connectors; i++)
			{
				bool connected;

				struct drm_mode_get_connector conn = {0};
				conn.connector_id = *((uint32_t*)res.connector_id_ptr + i);

				ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);

				conn.modes_ptr       = (uint64_t)malloc(conn.count_modes    * sizeof(drm_mode_modeinfo));
				conn.props_ptr       = (uint64_t)malloc(conn.count_props    * sizeof(uint32_t         ));
				conn.prop_values_ptr = (uint64_t)malloc(conn.count_props    * sizeof(uint64_t         ));
				conn.encoders_ptr    = (uint64_t)malloc(conn.count_encoders * sizeof(uint32_t         ));

				ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);

				if (!(conn.count_encoders < 1 || conn.count_modes < 1 || !conn.encoder_id || !conn.connection))
				{
					Output output = {};

					output.physical = { 
						.width  = conn.mm_width , 
						.height = conn.mm_height 
					};

					output.manufacturer = "N/A";
					output.model        = "N/A";

					drm_mode_modeinfo* mode_ptr = (drm_mode_modeinfo*)conn.modes_ptr;
					for (int a = conn.count_modes - 1; a >= 0; a--)
					{
						drm_mode_modeinfo* mode = mode_ptr + a;

						output.modes.push_back(Output::Mode{
							.resolution   = {
								.width  = mode->hdisplay,
								.height = mode->vdisplay
							},
							.refresh_rate = mode->vrefresh * 1000,
							.prefered     = mode->type & DRM_MODE_TYPE_PREFERRED,
							.current      = a == 0,
						});
					}

					Outputs::Add(output);

					CreateDumbBuffer(dri_fd, &res, &conn, i);
				}

				free((void*)conn.modes_ptr      );
				free((void*)conn.props_ptr      );
				free((void*)conn.prop_values_ptr);
				free((void*)conn.encoders_ptr   );
			}

			free((void*)res.fb_id_ptr       );
			free((void*)res.crtc_id_ptr     );
			free((void*)res.connector_id_ptr);
			free((void*)res.encoder_id_ptr  );

			ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);
			std::cout << "\n";
		}
	}
}

void Awning::Backend::DRM::Poll()
{
	memset(framebuffers[0].texture.buffer.u8, 0xEE, framebuffers[0].texture.size);
}

void Awning::Backend::DRM::Draw()
{
	memcpy(framebuffers[0].memoryMappedBuffer, framebuffers[0].texture.buffer.u8, framebuffers[0].texture.size);
}

Awning::WM::Texture Awning::Backend::DRM::Data()
{
	return framebuffers[0].texture;
}