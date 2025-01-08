#pragma once
#include "tga/tga.hpp"
#include "Scene.h"

class ShadowPass {
public:
    ShadowPass(tga::Interface &tgai, std::array<uint32_t, 2> resolution, const tga::VertexLayout &vertexLayout);
    ~ShadowPass();

    void upload(tga::CommandRecorder &recorder);
    void bind(tga::CommandRecorder &recorder, uint32_t nf);
    tga::Texture shadowMap() const;
    tga::RenderPass renderPass() const;
    void update(const ::Scene &scene);
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
