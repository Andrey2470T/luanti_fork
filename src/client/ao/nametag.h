#pragma once

#include <BasicIncludes.h>
#include "client/ui/hud_elements.h"

class Client;

class Nametag : public Waypoint
{
    bool show_backgrounds;
public:
    std::string text;
    img::color8 textcolor;
    std::optional<img::color8> bgcolor;

    Nametag(Client *client,
            const std::string &text,
            const img::color8 &textcolor,
            const std::optional<img::color8> &bgcolor,
            const v3f &pos);

    void updateBank(v3f newWorldPos) override;

    img::color8 getBgColor(bool use_fallback) const
	{
		if (bgcolor)
			return bgcolor.value();
		else if (!use_fallback)
            return img::color8();
        else if (textcolor.getLuminance() > 186)
			// Dark background for light text
            return img::color8(img::PF_RGBA8, 50, 50, 50, 50);
		else
			// Light background for dark text
            return img::color8(img::PF_RGBA8, 255, 255, 255, 50);
	}
};