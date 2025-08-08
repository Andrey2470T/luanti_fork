#pragma once

#include <Render/StreamTexture2D.h>
#include <Utils/ByteArray.h>

// Texture type specialized for saving large bulks of dynamic video data
// This should be used if the required data for packing generally exceeds the UBO limit size
class DataTexture
{
    u32 sampleSize; // in bytes

    std::vector<v2u> unfilledAreas; // free areas for new samples

    std::unique_ptr<render::StreamTexture2D> glTexture;
public:
    u32 sampleCount;
    u32 sampleDim; // image quad size (in pixels) of one sample
    u32 texDim; // image quad size (in samples) of whole texture

    DataTexture(const std::string &name, u32 sampleSize, u32 sampleCount, u32 sampleElemCount);

    u32 getSampleCount() const
    {
        return sampleCount;
    }

    render::StreamTexture2D *getGLTexture() const
    {
        return glTexture.get();
    }

    void addSample(ByteArray &data);
    void updateSample(u32 n, ByteArray &data);
    void removeSample(u32 n);

    void flush()
    {
        glTexture->flush();
    }
private:
    void calculateFreeAreas();
    v2u calculateCoords(u32 sampleN);
};
