#pragma once
#include <chrono>
#include <optional>

#include "tga/tga.hpp"
#include "Scene.h"
#include "ShadowPass.h"

class FogVolumeGenerationPass {
public:
    FogVolumeGenerationPass(tga::Interface &tgai, std::array<uint32_t, 3> resolution, const ShadowPass &sp);
    ~FogVolumeGenerationPass();
    FogVolumeGenerationPass(const FogVolumeGenerationPass &) = delete;
    FogVolumeGenerationPass &operator=(const FogVolumeGenerationPass &) = delete;

    void update(const Scene &scene, uint32_t nf, float historyFactor, float density, float constantDensity, float anisotropy, float absorption, float height, bool noise, float skyBlendRatio);
    void upload(tga::CommandRecorder &recorder) const;
    void execute(tga::CommandRecorder &recorder, uint32_t nf) const;
    tga::Buffer inputBuffer() const;
    tga::Texture scatteringVolume() const;
private:
    struct VolumeGenerationInputs {
        alignas(16) std::array<uint32_t, 3> resolution;
        alignas(16) glm::vec3 cameraPos;
        alignas(16) glm::vec3 cameraXAxis;
        alignas(16) glm::vec3 cameraYAxis;
        alignas(16) glm::vec3 cameraZAxis;
        alignas(4)  float zNear;
        alignas(4)  float zFar;
        alignas(16) glm::mat4 prevFrameVP;
        alignas(16) DirLight dirLight;
        alignas(4) float time;
        alignas(4) int frameNumber;
        alignas(4) float historyFactor;
        alignas(4) float density;
        alignas(4) float constantDensity;
        alignas(4) float anisotropy;
        alignas(4) float absorptionFactor;
        alignas(4) float height;
        alignas(4) bool noise;
        alignas(4) float skyBlendRatio;
    };

    tga::Interface *tgai;
    std::chrono::system_clock::time_point startTime;
    tga::ComputePass cp;
    tga::ComputePass accCp;
    std::array<uint32_t, 3> resolution;
    std::array<tga::Texture, 2> lightingVolumes;
    tga::Texture m_scatteringVolume;
    tga::Texture perlinNoise;
    tga::StagingBuffer generationInputsStaging;
    VolumeGenerationInputs *generationInputsData;
    tga::Buffer generationInputsBuffer;
    std::array<tga::InputSet, 2> generationInputs;
    std::array<tga::InputSet, 2> accumulationInputs;
    std::optional<glm::mat4> prevFrameVP;
};
