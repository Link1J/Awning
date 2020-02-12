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

std::vector<Awning::WM::Texture::Data> framebuffers;

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

			std::cout << "Size:" << "\n" 
					  << "    " << "Min: " << res.min_width << "," << res.min_height << "\n"
					  << "    " << "Max: " << res.max_width << "," << res.max_height << "\n";

			std::cout << "Connectors:" << "\n";

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

				if (conn.count_encoders < 1 || conn.count_modes < 1 || !conn.encoder_id || !conn.connection)
				{
					std::cout << "    " << i << ": Not connected " << "\n";
					connected = false;
				}
				else
				{
					std::cout << "    " << i << ": Connected " << "\n";
					connected = true;
				}

				switch (conn.connector_type)
				{
#define CASE(T) case DRM_MODE_CONNECTOR_##T: std::cout << "        " << "Type: " << #T << "\n"; break
					CASE(Unknown    );
					CASE(VGA        );
					CASE(DVII       );
					CASE(DVID       );
					CASE(DVIA       );
					CASE(Composite  );
					CASE(SVIDEO     );
					CASE(LVDS       );
					CASE(Component  );
					CASE(9PinDIN    );
					CASE(DisplayPort);
					CASE(HDMIA      );
					CASE(HDMIB      );
					CASE(TV         );
					CASE(eDP        );
					CASE(VIRTUAL    );
					CASE(DSI        );
					CASE(DPI        );
					CASE(WRITEBACK  );
#undef CASE
				}

				if (connected)
				{
					std::cout << "        " << "Display Size: " << conn.mm_width << "mm," << conn.mm_height << "mm" << "\n";

					std::cout << "        " << "Modes:" << "\n";

					drm_mode_modeinfo* mode_ptr = (drm_mode_modeinfo*)conn.modes_ptr;
					for (int a = conn.count_modes - 1; a >= 0; a--)
					{
						drm_mode_modeinfo* mode = mode_ptr + a;
						std::cout << "            " << mode->hdisplay << "x" << mode->vdisplay << " @ " << mode->vrefresh << "Hz" << "\n";
					}

					//CreateDumbBuffer(dri_fd, &res, &conn, i);
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

	/*framebuffer = Awning::WM::Texture::Data {
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
    };*/

	/*Output output {
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
	Outputs::Add(output);*/
}

void Awning::Backend::DRM::Poll()
{
	//memset(framebuffers[0].buffer.u8, 0xEE, framebuffers[0].size);
}

void Awning::Backend::DRM::Draw()
{
	//memcpy(framebufferMaped, framebuffers[0].buffer.u8, framebuffers[0].size);
}

Awning::WM::Texture::Data Awning::Backend::DRM::Data()
{
	return framebuffers[0];
}