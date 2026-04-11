// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2015 Aaron Suen <warr1024@gmail.com>

#include "guiscalingfilter.h"
#include "imagefilters.h"
#include "porting.h"
#include "settings.h"
#include "util/numeric.h"
#include <cstdio>
#include "client/renderingengine.h"
#include <Image.h>
#include <Texture.h>
#include <VideoDriver.h>

/* Maintain a static cache to store the images that correspond to textures
 * in a format that's manipulable by code.  Some platforms exhibit issues
 * converting textures back into images repeatedly, and some don't even
 * allow it at all.
 */
static std::map<io::path, video::Image *> g_imgCache;

/* Maintain a static cache of all pre-scaled textures.  These need to be
 * cleared as well when the cached images.
 */
static std::map<io::path, video::GLTexture *> g_txrCache;

/* Manually insert an image into the cache, useful to avoid texture-to-image
 * conversion whenever we can intercept it.
 */
void guiScalingCache(const io::path &key, video::VideoDriver *driver, video::Image *value)
{
	if (!g_settings->getBool("gui_scaling_filter"))
		return;

	if (g_imgCache.find(key) != g_imgCache.end())
		return; // Already cached.

	video::Image *copied = driver->createImage(value->getColorFormat(),
			value->getDimension());
	value->copyTo(copied);
	g_imgCache[key] = copied;
}

// Manually clear the cache, e.g. when switching to different worlds.
void guiScalingCacheClear()
{
	for (auto &it : g_imgCache) {
		if (it.second)
			it.second->drop();
	}
	g_imgCache.clear();
	for (auto &it : g_txrCache) {
		if (it.second)
			RenderingEngine::get_video_driver()->removeTexture(it.second);
	}
	g_txrCache.clear();
}

/* Get a cached, high-quality pre-scaled texture for display purposes.  If the
 * texture is not already cached, attempt to create it.  Returns a pre-scaled texture,
 * or the original texture if unable to pre-scale it.
 */
video::GLTexture *guiScalingResizeCached(video::VideoDriver *driver,
		video::GLTexture *src, const core::rect<s32> &srcrect,
		const core::rect<s32> &destrect)
{
	if (src == NULL)
		return src;

	if (!g_settings->getBool("gui_scaling_filter"))
		return src;

	// Calculate scaled texture name.
	char rectstr[200];
	porting::mt_snprintf(rectstr, sizeof(rectstr), "%d:%d:%d:%d:%d:%d",
		srcrect.UpperLeftCorner.X,
		srcrect.UpperLeftCorner.Y,
		srcrect.getWidth(),
		srcrect.getHeight(),
		destrect.getWidth(),
		destrect.getHeight());
	io::path origname = src->getName().getPath();
	io::path scalename = origname + "@guiScalingFilter:" + rectstr;

	// Search for existing scaled texture.
	auto it_txr = g_txrCache.find(scalename);
	video::GLTexture *scaled = (it_txr != g_txrCache.end()) ? it_txr->second : nullptr;
	if (scaled)
		return scaled;

	// Try to find the texture converted to an image in the cache.
	// If the image was not found, try to extract it from the texture.
	auto it_img = g_imgCache.find(origname);
	video::Image *srcimg = (it_img != g_imgCache.end()) ? it_img->second : nullptr;
	if (!srcimg) {
		// Download image from GPU
		srcimg = driver->createImageFromData(src->getColorFormat(),
			src->getSize(), src->lock(video::ETLM_READ_ONLY), false);
		src->unlock();
		g_imgCache[origname] = srcimg;
	}

	// Create a new destination image and scale the source into it.
	imageCleanTransparent(srcimg, 0);

	if (destrect.getWidth() <= 0 || destrect.getHeight() <= 0) {
		errorstream << "Attempted to scale texture to invalid size " << scalename.c_str() << std::endl;
		// Avoid log spam by reusing and displaying the original texture
		src->grab();
		g_txrCache[scalename] = src;
		return src;
	}
	video::Image *destimg = driver->createImage(src->getColorFormat(),
			core::dimension2d<u32>((u32)destrect.getWidth(),
			(u32)destrect.getHeight()));
	imageScaleNNAA(srcimg, srcrect, destimg);

	// Convert the scaled image back into a texture.
	scaled = driver->addTexture(scalename, destimg);
	destimg->drop();
	g_txrCache[scalename] = scaled;

	return scaled;
}

/* Convenience wrapper for guiScalingResizeCached that accepts parameters that
 * are available at GUI imagebutton creation time.
 */
video::GLTexture *guiScalingImageButton(video::VideoDriver *driver,
		video::GLTexture *src, s32 width, s32 height)
{
	if (src == NULL)
		return src;
	return guiScalingResizeCached(driver, src,
		core::rect<s32>(0, 0, src->getSize().Width, src->getSize().Height),
		core::rect<s32>(0, 0, width, height));
}

/* Replacement for driver->draw2DImage() that uses the high-quality pre-scaled
 * texture, if configured.
 */
void draw2DImageFilterScaled(video::VideoDriver *driver, video::GLTexture *txr,
		const core::rect<s32> &destrect, const core::rect<s32> &srcrect,
		const core::rect<s32> *cliprect, const video::SColor *const colors,
		bool usealpha)
{
	// 9-sliced images might calculate negative texture dimensions. Skip them.
	if (destrect.getWidth() <= 0 || destrect.getHeight() <= 0)
		return;

	// Attempt to pre-scale image in software in high quality.
	video::GLTexture *scaled = guiScalingResizeCached(driver, txr, srcrect, destrect);
	if (scaled == NULL)
		return;

	// Correct source rect based on scaled image.
	const core::rect<s32> mysrcrect = (scaled != txr)
		? core::rect<s32>(0, 0, destrect.getWidth(), destrect.getHeight())
		: srcrect;

	driver->draw2DImage(scaled, destrect, mysrcrect, cliprect, colors, usealpha);
}

void draw2DImage9Slice(video::VideoDriver *driver, video::GLTexture *texture,
		const core::rect<s32> &destrect, const core::rect<s32> &srcrect,
		const core::rect<s32> &middlerect, const core::rect<s32> *cliprect,
		const video::SColor *const colors)
{
	// `-x` is interpreted as `w - x`
	core::rect<s32> middle = middlerect;

	if (middlerect.LowerRightCorner.X < 0)
		middle.LowerRightCorner.X += srcrect.getWidth();
	if (middlerect.LowerRightCorner.Y < 0)
		middle.LowerRightCorner.Y += srcrect.getHeight();

	core::vector2di lower_right_offset = core::vector2di(srcrect.getWidth(),
			srcrect.getHeight()) - middle.LowerRightCorner;

	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 3; ++x) {
			core::rect<s32> src = srcrect;
			core::rect<s32> dest = destrect;

			switch (x) {
			case 0:
				dest.LowerRightCorner.X = destrect.UpperLeftCorner.X + middle.UpperLeftCorner.X;
				src.LowerRightCorner.X = srcrect.UpperLeftCorner.X + middle.UpperLeftCorner.X;
				break;

			case 1:
				dest.UpperLeftCorner.X += middle.UpperLeftCorner.X;
				dest.LowerRightCorner.X -= lower_right_offset.X;
				src.UpperLeftCorner.X += middle.UpperLeftCorner.X;
				src.LowerRightCorner.X -= lower_right_offset.X;
				break;

			case 2:
				dest.UpperLeftCorner.X = destrect.LowerRightCorner.X - lower_right_offset.X;
				src.UpperLeftCorner.X = srcrect.LowerRightCorner.X - lower_right_offset.X;
				break;
			}

			switch (y) {
			case 0:
				dest.LowerRightCorner.Y = destrect.UpperLeftCorner.Y + middle.UpperLeftCorner.Y;
				src.LowerRightCorner.Y = srcrect.UpperLeftCorner.Y + middle.UpperLeftCorner.Y;
				break;

			case 1:
				dest.UpperLeftCorner.Y += middle.UpperLeftCorner.Y;
				dest.LowerRightCorner.Y -= lower_right_offset.Y;
				src.UpperLeftCorner.Y += middle.UpperLeftCorner.Y;
				src.LowerRightCorner.Y -= lower_right_offset.Y;
				break;

			case 2:
				dest.UpperLeftCorner.Y = destrect.LowerRightCorner.Y - lower_right_offset.Y;
				src.UpperLeftCorner.Y = srcrect.LowerRightCorner.Y - lower_right_offset.Y;
				break;
			}

			draw2DImageFilterScaled(driver, texture, dest, src, cliprect, colors, true);
		}
	}
}
