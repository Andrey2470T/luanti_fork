#include "datatexture.h"
#include "renderer.h"

#define CALC_TEX_SIDE(area) \
    (u32)std::ceil(std::sqrt(area))


DataTexture::DataTexture(const std::string &name, u32 _sampleSize, u32 _sampleCount, u32 sampleElemCount)
    : sampleSize(_sampleSize), sampleCount(_sampleCount), sampleDim(CALC_TEX_SIDE(std::ceil(sampleSize/4.0f))),
    texDim(CALC_TEX_SIDE(sampleCount))
{
    calculateFreeAreas();

    render::TextureSettings settings;
    settings.minF = render::TMF_COUNT;
    settings.magF = render::TMAGF_COUNT;
    settings.isRenderTarget = false;

    u32 data_dim = CALC_TEX_SIDE(sampleSize*sampleCount/4);
    glTexture = std::make_unique<render::Texture2D>(name, data_dim, data_dim, img::PF_RGBA8, 0);
}

void DataTexture::addSample(ByteArray &data)
{
    assert(data.bytesCount() == sampleSize);
    sampleCount++;

    u8 *bytedata = reinterpret_cast<u8 *>(data.data());
    img::Image *tmpImg = new img::Image(img::PF_RGBA8, sampleDim, sampleDim, bytedata, false);

    if (unfilledAreas.empty()) {
        texDim = CALC_TEX_SIDE(sampleCount);

        u32 new_dim = CALC_TEX_SIDE(sampleSize*sampleCount/4);
        glTexture->resize(new_dim, new_dim, g_imgmodifier);

        calculateFreeAreas();
    }

    v2u targetCoords = unfilledAreas.at(0);
    unfilledAreas.erase(unfilledAreas.begin());

    glTexture->uploadSubData(targetCoords.X*sampleDim, targetCoords.Y*sampleDim, tmpImg, g_imgmodifier);

    delete tmpImg;
}

void DataTexture::updateSample(u32 n, ByteArray &data)
{
    assert(n <= sampleCount && data.bytesCount() == sampleSize);

    u8 *bytedata = reinterpret_cast<u8 *>(data.data());
    img::Image *tmpImg = new img::Image(img::PF_RGBA8, sampleDim, sampleDim, bytedata, false);

    v2u targetCoords = calculateCoords(n);

    glTexture->uploadSubData(targetCoords.X*sampleDim, targetCoords.Y*sampleDim, tmpImg, g_imgmodifier);

    delete tmpImg;
}

void DataTexture::removeSample(u32 n)
{
    assert(n <= sampleCount);

    sampleCount--;

    u8 *newData = new u8[sampleCount*sampleSize];
    u8 *curData = glTexture->downloadData().at(0)->getData();

    u32 prevSize = 0;
    if (n > 0) {
        v2u prevCoords = calculateCoords(n-1);
        prevSize = (prevCoords.Y * texDim + prevCoords.X) * sampleSize;
        memcpy(newData, curData, prevSize);
    }

    u32 nextSize = sampleCount*sampleSize-prevSize;

    memcpy(newData, curData+prevSize+sampleSize, nextSize);

    texDim = CALC_TEX_SIDE(sampleCount);

    calculateFreeAreas();
    img::Image *tmpImg = new img::Image(img::PF_RGBA8, texDim*sampleDim, texDim*sampleDim, newData, true);

    glTexture->uploadSubData(0, 0, tmpImg, g_imgmodifier);

    delete tmpImg;
}

void DataTexture::calculateFreeAreas()
{
    unfilledAreas.clear();

    for (u32 dimX = 0; dimX < texDim; dimX++)
        for (u32 dimY = 0; dimY < texDim; dimY++) {
            if ((dimY*texDim+dimX+1) > sampleCount)
                unfilledAreas.emplace_back(dimX, dimY);
        }
}

v2u DataTexture::calculateCoords(u32 sampleN)
{
    u32 y = (sampleN+1) / texDim;
    u32 x = sampleN % texDim;

    return v2u(x, y);
}
