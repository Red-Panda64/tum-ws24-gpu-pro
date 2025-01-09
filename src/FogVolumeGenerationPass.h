#pragma once

#include "tga/tga.hpp"
#include "Scene.h"
#include "ShadowPass.h"

class FogVolumeGenerationPass {
public:
    FogVolumeGenerationPass(tga::Interface &tgai, std::array<uint32_t, 3> resolution, const ShadowPass &sp);
    ~FogVolumeGenerationPass();
    FogVolumeGenerationPass(const FogVolumeGenerationPass &) = delete;
    FogVolumeGenerationPass &operator=(const FogVolumeGenerationPass &) = delete;

    void upload(tga::CommandRecorder &recorder) const;
    void execute(tga::CommandRecorder &recorder, uint32_t nf) const;
    tga::ComputePass computePass() const;
    tga::InputSet createInputSet(tga::RenderPass rp) const;
private:
    struct VolumeGenerationInputs {
        alignas(16) std::array<uint32_t, 3> resolution;
    };

    tga::Interface *tgai;
    tga::ComputePass cp;
    std::array<uint32_t, 3> resolution;
    std::array<tga::Texture, 2> lightingVolumes;
    tga::Texture scatteringVolume;
    tga::Buffer generationInputsBuffer;
    std::array<tga::InputSet, 2> generationInputs;
};
