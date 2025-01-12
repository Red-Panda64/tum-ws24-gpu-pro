#pragma once

#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"

#include "Camera.h"

struct DirLight
{
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
};

struct PointLight
{
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 attenuationFactors; // constant, linear and quadratic in order
};

// As they will be passed alongside the uniform the array size must be pre-specified
#define MAX_NR_OF_POINT_LIGHTS 16
struct SceneUniformBuffer
{
    alignas(16) glm::mat4 projectionView;
    alignas(16) glm::vec3 cameraPos;
    alignas(16) DirLight dirLight;
    alignas(16) PointLight pointLights[MAX_NR_OF_POINT_LIGHTS];
    alignas(16) int nrPointLights;
    alignas(16) float ambientFactor;
};

class Scene
{
public:
    Scene(tga::Interface& tgai);
    void initCamera(const glm::vec3& pos, const glm::quat& orientation);
    void setDirLight(const glm::vec3& direction, const glm::vec3& color);
    void addPointLight(const glm::vec3& position, const glm::vec3& color, const glm::vec3& attenuationFactors);
    void setAmbientFactor(float ambientFactor);
    void prepareSceneUniformBuffer(tga::Interface& tgai);
    void updateSceneBufferCameraData(float aspectRatio);
    void bufferUpload(tga::CommandRecorder& recorder);
    void moveCamera(const glm::vec3& direction, float deltaTime, float speed);
    void moveCameraXDir(float direction, float deltaTime, float speed);
    void moveCameraYDir(float direction, float deltaTime, float speed);
    void moveCameraZDir(float direction, float deltaTime, float speed);
    void rotateCameraWithMouseInput(double xPos, double yPos);
    void updateCameraLastMousePos(double x, double y);
    tga::Buffer buffer() const;
    const glm::mat4 &viewProjection() const;
    const DirLight &dirLight() const;
    const Camera &camera() const;
private:
    Camera m_camera;
    tga::StagingBuffer sceneStagingBuffer;
    SceneUniformBuffer* pSceneStagingBuffer;
    tga::Buffer sceneBuffer;
    // Information regarding the Uniform Buffer needed for data uploading (useful for partial updates) TODO: NOT USED YET
    // const size_t cameraDataSize = sizeof(pSceneStagingBuffer->view) + sizeof(pSceneStagingBuffer->projection);
    // const size_t cameraDataOffset = offsetof(SceneUniformBuffer, view);
};