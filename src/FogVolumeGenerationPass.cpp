#include <cstring>
#include <cmath>

#include "FogVolumeGenerationPass.h"
#include "util.h"

FogVolumeGenerationPass::FogVolumeGenerationPass(tga::Interface &tgai, std::array<uint32_t, 3> resolution, const ShadowPass &sp) : tgai{&tgai}, startTime{std::chrono::system_clock::now()}, resolution{resolution}
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
    m_scatteringVolume = tgai.createTexture(texInfo);
    tgai.free(zeroStaging);

    generationInputsStaging = tgai.createStagingBuffer({ sizeof(VolumeGenerationInputs), nullptr });
    generationInputsData = reinterpret_cast<VolumeGenerationInputs *>(tgai.getMapping(generationInputsStaging));
    generationInputsData->resolution = resolution;
    generationInputsBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(VolumeGenerationInputs), generationInputsStaging });

    auto volumeGenerationShader = tga::loadShader("../shaders/volumetric_fog_comp.spv", tga::ShaderType::compute, tgai);
    cp = tgai.createComputePass({ volumeGenerationShader, tga::InputLayout{ { tga::BindingType::storageImage, tga::BindingType::sampler, tga::BindingType::uniformBuffer, tga::BindingType::uniformBuffer, tga::BindingType::sampler, tga::BindingType::sampler } } });
    tgai.free(volumeGenerationShader);

    perlinNoise = tga::loadTexture("../assets/textures/perlin.png", tga::Format::r32_sfloat, tga::SamplerMode::linear, tga::AddressMode::repeat, tgai, false);

    generationInputs[0] = tgai.createInputSet({ cp, { tga::Binding(lightingVolumes[0], 0), tga::Binding(lightingVolumes[1], 1), tga::Binding(generationInputsBuffer, 2), tga::Binding(sp.inputBuffer(), 3), tga::Binding(sp.shadowMap(), 4), tga::Binding(perlinNoise, 5) }, 0 });
    generationInputs[1] = tgai.createInputSet({ cp, { tga::Binding(lightingVolumes[1], 0), tga::Binding(lightingVolumes[0], 1), tga::Binding(generationInputsBuffer, 2), tga::Binding(sp.inputBuffer(), 3), tga::Binding(sp.shadowMap(), 4), tga::Binding(perlinNoise, 5) }, 0 });

    auto volumeAccumulationShader = tga::loadShader("../shaders/volumetric_fog_raymarch_comp.spv", tga::ShaderType::compute, tgai);
    accCp = tgai.createComputePass({ volumeAccumulationShader, tga::InputLayout{ { tga::BindingType::storageImage, tga::BindingType::storageImage, tga::BindingType::uniformBuffer } } });
    /* reusing generation inputs */
    accumulationInputs[0] = tgai.createInputSet({ accCp, { tga::Binding(lightingVolumes[0], 0), tga::Binding(m_scatteringVolume, 1), tga::Binding(generationInputsBuffer, 2) }, 0 });
    accumulationInputs[1] = tgai.createInputSet({ accCp, { tga::Binding(lightingVolumes[1], 0), tga::Binding(m_scatteringVolume, 1), tga::Binding(generationInputsBuffer, 2) }, 0 });
    tgai.free(volumeAccumulationShader);
}

FogVolumeGenerationPass::~FogVolumeGenerationPass()
{
    tgai->free(lightingVolumes[0]);
    tgai->free(lightingVolumes[1]);
    tgai->free(m_scatteringVolume);
    tgai->free(generationInputs[0]);
    tgai->free(generationInputs[1]);
    tgai->free(generationInputsBuffer);
    tgai->free(generationInputsStaging);
    tgai->free(accumulationInputs[0]);
    tgai->free(accumulationInputs[1]);
    tgai->free(cp);
}

void FogVolumeGenerationPass::update(const Scene &scene, uint32_t nf, float historyFactor, float density, float constantDensity, float anisotropy, float absorption, float height, bool noise, float skyBlendRatio)
{
    float time = std::fmod(std::chrono::duration_cast<std::chrono::duration<float>>(startTime - std::chrono::system_clock::now()).count() / 60.0f, 1.0f);

    const Camera &camera = scene.camera();
    generationInputsData->cameraPos = camera.getPosition();

    glm::mat4 projection = camera.projection();
    float projWidth = projection[0][0];
    float projHeight = projection[1][1];

    glm::mat4 vp = scene.viewProjection();
    glm::mat4 invView = glm::inverse(camera.view());
    generationInputsData->cameraXAxis = invView * glm::vec4(1.0f / projWidth, 0, 0, 0);
    generationInputsData->cameraYAxis = invView * glm::vec4(0, 1.0f / projHeight, 0, 0);
    generationInputsData->cameraZAxis = invView * glm::vec4(0, 0, -1, 0);
    generationInputsData->zNear = camera.zNear();
    generationInputsData->zFar  = camera.zFar();
    generationInputsData->dirLight = scene.dirLight();
    generationInputsData->frameNumber = nf;
    if(prevFrameVP) {
        generationInputsData->prevFrameVP = *prevFrameVP;
    } else {
        generationInputsData->prevFrameVP = vp;
    }
    generationInputsData->resolution = resolution;
    generationInputsData->time = time;
    generationInputsData->historyFactor = historyFactor;
    generationInputsData->density = density;
    generationInputsData->constantDensity = constantDensity;
    generationInputsData->anisotropy = anisotropy;
    generationInputsData->absorptionFactor = absorption;
    generationInputsData->height = height;
    generationInputsData->noise = noise;
    generationInputsData->skyBlendRatio = skyBlendRatio;
    prevFrameVP = vp;
}

void FogVolumeGenerationPass::upload(tga::CommandRecorder &recorder) const
{
    recorder.bufferUpload(generationInputsStaging, generationInputsBuffer, sizeof(VolumeGenerationInputs));
}

void FogVolumeGenerationPass::execute(tga::CommandRecorder &recorder, uint32_t nf) const
{
    recorder.setComputePass(cp);
    recorder.bindInputSet(generationInputs[nf % 2]);
    recorder.dispatch(ceilDiv(resolution[0], 4u) , ceilDiv(resolution[1], 4u), ceilDiv(resolution[2], 4u));

    recorder.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::ComputeShader);
    recorder.setComputePass(accCp);
    recorder.bindInputSet(accumulationInputs[nf % 2]);
    recorder.dispatch(ceilDiv(resolution[0], 4u) , ceilDiv(resolution[1], 4u), 1);
}

tga::Buffer FogVolumeGenerationPass::inputBuffer() const
{
    return generationInputsBuffer;
}

tga::Texture FogVolumeGenerationPass::scatteringVolume() const
{
    return m_scatteringVolume;
}
