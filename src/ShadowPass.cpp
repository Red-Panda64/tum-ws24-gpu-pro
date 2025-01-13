#include "ShadowPass.h"
#include "tga/tga_utils.hpp"

static glm::vec3 dehomogenize(const glm::vec4 v) {
    return glm::vec3(v.x / v.w, v.y / v.w, v.z / v.w);
}

ShadowPass::ShadowPass(tga::Interface &tgai, std::array<uint32_t, 2> resolution, const tga::VertexLayout &vertexLayout) : tgai{&tgai} {
    hShadowMap = tgai.createTexture({resolution[0], resolution[1], tga::Format::r32_sfloat, tga::SamplerMode::nearest,
                     tga::AddressMode::clampBorder}); // TODO: how to set border color?

    auto shadow_vs = tga::loadShader("../shaders/shadow_vert.spv", tga::ShaderType::vertex, tgai);
    auto shadow_fs = tga::loadShader("../shaders/shadow_frag.spv", tga::ShaderType::fragment, tgai);

    tga::SetLayout sceneSetLayout = tga::SetLayout{ {tga::BindingType::uniformBuffer} };
    tga::SetLayout objectSetLayout = tga::SetLayout{ {tga::BindingType::uniformBuffer} };
    tga::InputLayout descriptorLayout = tga::InputLayout{ sceneSetLayout, objectSetLayout };

    auto shadowrpInfo = tga::RenderPassInfo{shadow_vs, shadow_fs, hShadowMap} // Unfortunately, this "render target" is essentially a redundant depth buffer
        .setClearOperations(tga::ClearOperation::all)
        .setPerPixelOperations(tga::PerPixelOperations{}.setDepthCompareOp(tga::CompareOperation::lessEqual))
        .setRasterizerConfig(tga::RasterizerConfig{}.setFrontFace(tga::FrontFace::counterclockwise).setCullMode(tga::CullMode::back))
        .setInputLayout(descriptorLayout)
        .setVertexLayout(vertexLayout);
    rp = tgai.createRenderPass(shadowrpInfo);
    sceneDataStaging = tgai.createStagingBuffer({sizeof(Scene)});
    sceneData = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(Scene) });
    scene = reinterpret_cast<Scene *>(tgai.getMapping(sceneDataStaging));
    sceneSet = tgai.createInputSet({ rp, { tga::Binding(sceneData, 0, 0) }, 0 });
    tgai.free(shadow_vs);
    tgai.free(shadow_fs);
}

ShadowPass::~ShadowPass()
{
    tgai->free(hShadowMap);
    tgai->free(sceneData);
    tgai->free(sceneDataStaging);
    tgai->free(rp);
}

void ShadowPass::upload(tga::CommandRecorder &recorder) const
{
    recorder.bufferUpload(sceneDataStaging, sceneData, sizeof(Scene));
}

void ShadowPass::bind(tga::CommandRecorder &recorder, uint32_t nf) const
{
    recorder.setRenderPass(rp, nf, { 1.0f }, 1.0f);
    recorder.bindInputSet(sceneSet);
}

tga::Texture ShadowPass::shadowMap() const
{
    return hShadowMap;
}

tga::Buffer ShadowPass::inputBuffer() const
{
    return sceneData;
}

tga::RenderPass ShadowPass::renderPass() const
{
    return rp;
}

void ShadowPass::update(const ::Scene &scene, float shadowNearDistance, float shadowFarDistance)
{
    const glm::mat4 &vp = scene.viewProjection();
    glm::mat4 invVp = glm::inverse(vp);
    glm::vec3 lightDir = normalize(scene.dirLight().direction);
    std::array<glm::vec3, 3> axes;
    axes[2] = lightDir;
    glm::vec3 lookDir = glm::normalize(dehomogenize(invVp * glm::vec4(0.0, 0.0, 1.0, 0.0)));
    axes[0] = lookDir - glm::dot(lightDir, lookDir) * lightDir;
    float axis0Norm = glm::l2Norm(axes[0]);
    if(axis0Norm >= 0.05f) {
        axes[0] = axes[0] / axis0Norm;
        axes[1] = glm::normalize(glm::cross(lightDir, axes[0]));
    } else {
        axes[0] = glm::cross(lightDir, glm::vec3(1.0, 0.0, 0.0));
        axis0Norm = glm::l2Norm(axes[0]);
        if(axis0Norm < 0.5f) {
            axes[0] = glm::normalize(glm::cross(lightDir, glm::vec3(0.0, 1.0, 0.0)));
        } else {
            axes[0] /= axis0Norm;
        }
        axes[1] = glm::normalize(glm::cross(lightDir, axes[0]));
    }
    glm::vec3 frustumVerts[8] = {
        { 1.0,  1.0, 0.0},
        {-1.0,  1.0, 0.0},
        { 1.0, -1.0, 0.0},
        {-1.0, -1.0, 0.0},
        { 1.0,  1.0, 1.0},
        {-1.0,  1.0, 1.0},
        { 1.0, -1.0, 1.0},
        {-1.0, -1.0, 1.0},
    };
    std::array<std::array<float, 2>, 3> axisExtents {
        std::array<float, 2> { std::numeric_limits<float>::max(), std::numeric_limits<float>::min() },
        std::array<float, 2> { std::numeric_limits<float>::max(), std::numeric_limits<float>::min() },
        std::array<float, 2> { std::numeric_limits<float>::max(), std::numeric_limits<float>::min() },
    };
    for(size_t i = 0; i < 8; i++) {
        frustumVerts[i] = dehomogenize(invVp * glm::vec4(frustumVerts[i], 1.0));
    }

    for(size_t i = 0; i < 4; i++) {
        glm::vec3 near = frustumVerts[i];
        glm::vec3 far = frustumVerts[i + 4];
        frustumVerts[i] = glm::mix(near, far, shadowNearDistance);
        frustumVerts[i + 4] = glm::mix(near, far, shadowFarDistance);
    }

    for(size_t i = 0; i < 8; i++) {
        for(size_t j = 0; j < 3; j++) {
            float projection = glm::dot(frustumVerts[i], axes[j]);
            axisExtents[j][0] = std::min(axisExtents[j][0], projection);
            axisExtents[j][1] = std::max(axisExtents[j][1], projection);
        }
    }

    // extend towards lightsource
    axisExtents[2][0] -= 200.0f;

    glm::mat4 perspective = glm::mat4(0.0f);
    perspective[0][0] = 2.0f / (axisExtents[0][1] - axisExtents[0][0]);
    perspective[1][1] = 2.0f / (axisExtents[1][1] - axisExtents[1][0]);
    perspective[2][2] = 1.0f / (axisExtents[2][1] - axisExtents[2][0]);
    perspective[3][0] = (axisExtents[0][0] + axisExtents[0][1]) / (axisExtents[0][0] - axisExtents[0][1]);
    perspective[3][1] = (axisExtents[1][0] + axisExtents[1][1]) / (axisExtents[1][0] - axisExtents[1][1]);
    perspective[3][2] =  axisExtents[2][0]                      / (axisExtents[2][0] - axisExtents[2][1]);
    perspective[3][3] = 1.0f;

    glm::mat4 view = glm::mat4(0.0f);
    view[3][3] = 1.0f;
    for(size_t i = 0; i < 3; i++) {
        for(size_t j = 0; j < 3; j++) {
            // change of basis is transposed, inverting it, because the vectors are orthogonal
            view[i][j] = axes[j][i];
        }
    }
    this->scene->viewProjection = perspective * view;
}
