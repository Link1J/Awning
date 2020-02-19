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
#include <vector>

#include "drm.hpp"
#include "manager.hpp"

#include "wm/output.hpp"

struct Connector
{
	int                     framebuffer = -1     ;
	Awning::WM::Output::ID  output      = -1     ;
	Awning::WM::Texture     texture              ;
	int                     prefered    = -1     ;
	uint32_t                handle      = -1     ;
	uint32_t                encoder     = -1     ;
	uint32_t                id          = -1     ;
	drm_mode_modeinfo     * modes       = nullptr;
	int                     current     = -1     ;
};

struct Card
{
	int                    id        ;
	int                    dri_fd    ;
	std::vector<Connector> connectors;
};

std::vector<Card> cards;

void CheckData();
void CreateFramebuffer(int card, int connector);
void SetMode(int card, int connector, int mode);

void Awning::Backend::DRM::Start()
{
	std::vector<std::string> files;

	for(auto& di : std::filesystem::directory_iterator("/dev/dri"))
	{
		auto p = di.path();
		std::string s = p.filename();

		if (s.starts_with("card"))
			files.push_back(p.c_str());
	}

	cards.resize(files.size());
	for (int a = 0; a < files.size(); a++)
	{
		cards[a].id     = a                                         ;
		cards[a].dri_fd = open(files[a].c_str(), O_RDWR | O_CLOEXEC);
	}

	CheckData();
}

void Awning::Backend::DRM::Poll()
{
}

void Awning::Backend::DRM::Draw()
{
}

Awning::Backend::Displays Awning::Backend::DRM::GetDisplays()
{
	Displays displays;

	for (auto& card : cards)
	{
		for (auto& connector : card.connectors)
		{
			if (connector.output == -1)
				continue;

			Display display;
			display.output  = connector.output ;
			display.texture = connector.texture;
			display.mode    = connector.current;
			displays.push_back(display);
		}
	}

	return displays;
}

void CheckData()
{
	using namespace Awning::WM;

	for (auto& card : cards)
	{
		ioctl(card.dri_fd, DRM_IOCTL_SET_MASTER, 0);

		struct drm_mode_card_res res = {0};
		ioctl(card.dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);

		res.fb_id_ptr        = (uint64_t)malloc(res.count_fbs        * sizeof(uint32_t));
		res.crtc_id_ptr      = (uint64_t)malloc(res.count_crtcs      * sizeof(uint32_t));
		res.connector_id_ptr = (uint64_t)malloc(res.count_connectors * sizeof(uint32_t));
		res.encoder_id_ptr   = (uint64_t)malloc(res.count_encoders   * sizeof(uint32_t));

		ioctl(card.dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);

		if (card.connectors.size() != res.count_connectors)
			card.connectors.resize(res.count_connectors);

		for (int i = 0; i < res.count_connectors; i++)
		{
			struct drm_mode_get_connector conn = {0};
			conn.connector_id = *((uint32_t*)res.connector_id_ptr + i);

			ioctl(card.dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);

			conn.modes_ptr       = (uint64_t)malloc(conn.count_modes    * sizeof(drm_mode_modeinfo));
			conn.props_ptr       = (uint64_t)malloc(conn.count_props    * sizeof(uint32_t         ));
			conn.prop_values_ptr = (uint64_t)malloc(conn.count_props    * sizeof(uint64_t         ));
			conn.encoders_ptr    = (uint64_t)malloc(conn.count_encoders * sizeof(uint32_t         ));
			
			ioctl(card.dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);

			bool connected = !(conn.count_encoders < 1 || conn.count_modes < 1 || !conn.encoder_id || !conn.connection);

			if (card.connectors[i].modes)
				delete card.connectors[i].modes;

			card.connectors[i].id      = *((uint32_t*)res.connector_id_ptr + i);
			card.connectors[i].encoder = conn.encoder_id                       ;
			card.connectors[i].modes   = (drm_mode_modeinfo*)conn.modes_ptr    ;

			if (connected)
			{
				if (card.connectors[i].output == -1)
				{
					card.connectors[i].output = Output::Create();
				}
				Output::ID output = card.connectors[i].output;

				if (Output::Get::NumberOfModes(output) == conn.count_modes)
					Output::Set::NumberOfModes(output, conn.count_modes);

				Output::Set::Manufacturer(output, "N/A"                        );
				Output::Set::Model       (output, "N/A"                        );
				Output::Set::Size        (output, conn.mm_width, conn.mm_height);

				drm_mode_modeinfo* mode_ptr = (drm_mode_modeinfo*)conn.modes_ptr;
				for (int a = 0; a < conn.count_modes; a++)
				{
					drm_mode_modeinfo* mode = mode_ptr + a;

					Output::Set::Mode::Resolution (output, a, mode->hdisplay, mode->vdisplay      );
					Output::Set::Mode::RefreshRate(output, a, mode->vrefresh * 1000               );
					Output::Set::Mode::Prefered   (output, a, mode->type & DRM_MODE_TYPE_PREFERRED);

					if (mode->type & DRM_MODE_TYPE_PREFERRED)
						card.connectors[i].prefered = a;
				}
				
				if (card.connectors[i].texture.red.size != 8)
					CreateFramebuffer(card.id, i);
			}
			else
			{
				if (card.connectors[i].output != -1)
				{
					Awning::WM::Output::Destory(card.connectors[i].output);
					card.connectors[i].output = -1;
				}
			}

			free((void*)conn.props_ptr      );
			free((void*)conn.prop_values_ptr);
			free((void*)conn.encoders_ptr   );
		}

		free((void*)res.fb_id_ptr       );
		free((void*)res.crtc_id_ptr     );
		free((void*)res.connector_id_ptr);
		free((void*)res.encoder_id_ptr  );

		ioctl(card.dri_fd, DRM_IOCTL_DROP_MASTER, 0);
	}
}

void CreateFramebuffer(int cardID, int connectorID)
{
	Card     & card      = cards          [cardID     ];
	Connector& connector = card.connectors[connectorID];

	connector.texture.buffer.pointer = 0 ;
	connector.texture.width          = 0 ;
	connector.texture.height         = 0 ;
	connector.texture.bytesPerLine   = 0 ;
	connector.texture.size           = 0 ;
	connector.texture.bitsPerPixel   = 32;
	connector.texture.red            = { .size = 8, .offset = 16 };
	connector.texture.green          = { .size = 8, .offset =  8 };
	connector.texture.blue           = { .size = 8, .offset =  0 };
	connector.texture.alpha          = { .size = 0, .offset = 24 };
}

void SetMode(int cardID, int connectorID, int modeID)
{
	Card     & card      = cards          [cardID     ];
	Connector& connector = card.connectors[connectorID];

	if (connector.output == -1)
		return;

	if (connector.handle != -1)
	{
		drm_mode_destroy_dumb destory = { connector.handle };
		ioctl(card.dri_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory);
		connector.handle = -1;
	}

	struct drm_mode_create_dumb create_dumb = {0};
	struct drm_mode_map_dumb    map_dumb    = {0};
	struct drm_mode_fb_cmd      cmd_dumb    = {0};
	struct drm_mode_get_encoder enc         = {0};
	struct drm_mode_crtc        crtc        = {0};

	auto [width, height] = Awning::WM::Output::Get::Mode::Resolution(connector.output, modeID);

	create_dumb.width  = width;
	create_dumb.height = height;
	create_dumb.bpp    = connector.texture.bitsPerPixel;
	create_dumb.flags  = 0;
	create_dumb.pitch  = 0;
	create_dumb.size   = 0;
	create_dumb.handle = 0;
	ioctl(card.dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);

	connector.texture.width          = create_dumb.width ;
	connector.texture.height         = create_dumb.height;
	connector.texture.bytesPerLine   = create_dumb.pitch ;
	connector.texture.size           = create_dumb.size  ;

	cmd_dumb.width   = create_dumb.width ;
	cmd_dumb.height  = create_dumb.height;
	cmd_dumb.bpp     = create_dumb.bpp   ;
	cmd_dumb.pitch   = create_dumb.pitch ;
	cmd_dumb.depth   = 24                ;
	cmd_dumb.handle  = create_dumb.handle;
	ioctl(card.dri_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb);

	map_dumb.handle = create_dumb.handle;
	ioctl(card.dri_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb);

	connector.texture.buffer.pointer = (uint8_t*)mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, card.dri_fd, map_dumb.offset);

	enc.encoder_id = connector.encoder;
	ioctl(card.dri_fd, DRM_IOCTL_MODE_GETENCODER, &enc);

	crtc.crtc_id = enc.crtc_id;
	ioctl(card.dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc);

	crtc.fb_id              = cmd_dumb.fb_id;
	crtc.set_connectors_ptr = connector.id;
	crtc.count_connectors   = 1;
	crtc.mode               = connector.modes[modeID];
	crtc.mode_valid         = 1;

	ioctl(card.dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc);

	connector.current = modeID;
}