#include <cstring>

#include "FogVolumeGenerationPass.h"
#include "util.h"

FogVolumeGenerationPass::FogVolumeGenerationPass(tga::Interface &tgai, std::array<uint32_t, 3> resolution, const ShadowPass &sp) : tgai{&tgai}, resolution{resolution}
{
    size_t textureMemorySize = resolution[0] * resolution[1] * resolution[2] * sizeof(glm::vec4);
    tga::StagingBuffer zeroStaging = tgai.createStagingBuffer({textureMemorySize});
    void *zeros = tgai.getMapping(zeroStaging);
    std::memset(zeros, 0, textureMemorySize);
    tga::TextureInfo texInfo{ resolution[0], resolution[1], tga::Format::r32g32b32a32_sfloat,
                              tga::SamplerMode::linear, tga::AddressMode::clampEdge,
                              tga::TextureType::_3D, resolution[2], zeroStaging, 0 };
    lightingVolumes[0] = tgai.createTexture(texInfo);
    lightingVolumes[1] = tgai.createTexture(texInfo);
    scatteringVolume = tgai.createTexture(texInfo);
    tgai.free(zeroStaging);

    VolumeGenerationInputs inputs { .resolution = resolution };
    tga::StagingBuffer inputStaging = tgai.createStagingBuffer({ sizeof(inputs), reinterpret_cast<uint8_t *>(&inputs) });
    generationInputsBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(inputs), inputStaging });

    auto volumeGenerationShader = tga::loadShader("../shaders/volumetric_fog_comp.spv", tga::ShaderType::vertex, tgai);

    cp = tgai.createComputePass({ volumeGenerationShader, tga::InputLayout{ { tga::BindingType::storageImage, tga::BindingType::sampler, tga::BindingType::uniformBuffer, tga::BindingType::uniformBuffer, tga::BindingType::sampler } } });
    tgai.free(volumeGenerationShader);

    generationInputs[0] = tgai.createInputSet({ cp, { tga::Binding(lightingVolumes[0]), tga::Binding(lightingVolumes[1]), tga::Binding(generationInputsBuffer), tga::Binding(sp.inputBuffer()), tga::Binding(sp.shadowMap()) }, 0 });
    generationInputs[1] = tgai.createInputSet({ cp, { tga::Binding(lightingVolumes[1]), tga::Binding(lightingVolumes[0]), tga::Binding(generationInputsBuffer), tga::Binding(sp.inputBuffer()), tga::Binding(sp.shadowMap()) }, 0 });
}

FogVolumeGenerationPass::~FogVolumeGenerationPass()
{
    tgai->free(lightingVolumes[0]);
    tgai->free(lightingVolumes[1]);
    tgai->free(scatteringVolume);
    tgai->free(generationInputs[0]);
    tgai->free(generationInputs[1]);
    tgai->free(cp);
}

void FogVolumeGenerationPass::execute(tga::CommandRecorder &recorder, uint32_t nf) const
{
    recorder.setComputePass(cp);
    recorder.bindInputSet(generationInputs[nf % 2]);
    recorder.dispatch(ceilDiv(resolution[0], 4u) , ceilDiv(resolution[1], 4u), ceilDiv(resolution[2], 4u));
}

tga::InputSet FogVolumeGenerationPass::createInputSet(tga::RenderPass rp) const
{
    /* TODO: determine set numbers in a better way */
    return tgai->createInputSet({rp, { tga::Binding(scatteringVolume, 0, 0) }, 4});
}
