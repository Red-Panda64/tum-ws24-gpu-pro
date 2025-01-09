#pragma once
#include "tga/tga.hpp"
#include "Scene.h"

class ShadowPass {
public:
    ShadowPass(tga::Interface &tgai, std::array<uint32_t, 2> resolution, const tga::VertexLayout &vertexLayout);
    ~ShadowPass();
    ShadowPass(const ShadowPass&) = delete;
    ShadowPass &operator=(const ShadowPass&) = delete;

    void upload(tga::CommandRecorder &recorder) const;
    void bind(tga::CommandRecorder &recorder, uint32_t nf) const;
    tga::Texture shadowMap() const;
    tga::RenderPass renderPass() const;
    /* near and far distance ar given as fractions of the view distance */
    void update(const ::Scene &scene, float shadowNearDistance, float shadowFarDistance);
    tga::InputSet createInputSet(tga::RenderPass rp) const;
private:
    struct Scene {
        glm::mat4 viewProjection;
    } *scene;

    tga::Interface *tgai;
    tga::RenderPass rp;
    tga::Texture hShadowMap;
    tga::Buffer sceneData;
    tga::StagingBuffer sceneDataStaging;
    tga::InputSet sceneSet;
};
