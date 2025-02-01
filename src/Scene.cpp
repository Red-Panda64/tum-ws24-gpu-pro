#include "Scene.h"

Scene::Scene(tga::Interface& tgai)
{
	prepareSceneUniformBuffer(tgai);
}

void Scene::initCamera(const glm::vec3& pos, float pitch, float yaw, float roll)
{
	m_camera = Camera(pos, pitch, yaw, roll);
}

void Scene::setDirLight(const glm::vec3& direction, const glm::vec3& color)
{
	pSceneStagingBuffer->dirLight.direction = direction;
	pSceneStagingBuffer->dirLight.color = color;
}

void Scene::addPointLight(const glm::vec3& position, const glm::vec3& color, const glm::vec3& attenuationFactors)
{
	if(pSceneStagingBuffer->nrPointLights == MAX_NR_OF_POINT_LIGHTS)
	{
		std::cout << "Maximum number of point lights is reached. If you want more lights please change MAX_NR_OF_POINT_LIGHTS\n";
		return;
	}

	pSceneStagingBuffer->pointLights[pSceneStagingBuffer->nrPointLights] = PointLight{position, color, attenuationFactors};
	++pSceneStagingBuffer->nrPointLights;
}

void Scene::setAmbientFactor(float ambientFactor)
{
	pSceneStagingBuffer->ambientFactor = ambientFactor;
}

void Scene::prepareSceneUniformBuffer(tga::Interface& tgai)
{
	sceneStagingBuffer = tgai.createStagingBuffer({ sizeof(SceneUniformBuffer) });
	// Map it and store to update it in runtime
	pSceneStagingBuffer = (SceneUniformBuffer*)(tgai.getMapping(sceneStagingBuffer));
	new(pSceneStagingBuffer) SceneUniformBuffer {
        .projectionView = glm::mat4(1.0f),
        .invProjectionView = glm::mat4(1.0f),
        .cameraPos = glm::vec3(0.0f),
        .dirLight = { .direction = glm::vec3(0.0f, -1.0f, 0.0f), .color = glm::vec3(0.7f, 0.7f, 0.7f) },
        .pointLights = {},
        .zNear = 0.0f,
        .zFar = 0.0f,
        .nrPointLights = 0,
        .ambientFactor = 0.0f,
        .viewport = glm::uvec2(0, 0),
    };
	// The actual buffer in GPU
	sceneBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(SceneUniformBuffer), sceneStagingBuffer });
}

void Scene::updateSceneBufferCameraData(glm::uvec2 viewport)
{
	m_camera.setViewport(viewport);
	pSceneStagingBuffer->projectionView = m_camera.projection() * m_camera.view();
	pSceneStagingBuffer->invProjectionView = glm::inverse(pSceneStagingBuffer->projectionView);
	pSceneStagingBuffer->cameraPos = m_camera.getPosition();
	pSceneStagingBuffer->zNear = m_camera.zNear();
	pSceneStagingBuffer->zFar = m_camera.zFar();
	pSceneStagingBuffer->viewport = glm::vec2(viewport.x, viewport.y);
}

void Scene::bufferUpload(tga::CommandRecorder& recorder)
{
	recorder.bufferUpload(sceneStagingBuffer, sceneBuffer, sizeof(SceneUniformBuffer));
}

void Scene::moveCamera(const glm::vec3& direction, float deltaTime, float speed)
{
	m_camera.move(direction, deltaTime, speed);
}

void Scene::moveCameraXDir(float direction, float deltaTime, float speed)
{
	m_camera.moveXDir(direction, deltaTime, speed);
}

void Scene::moveCameraYDir(float direction, float deltaTime, float speed)
{
	m_camera.moveYDir(direction, deltaTime, speed);
}

void Scene::moveCameraZDir(float direction, float deltaTime, float speed)
{
	m_camera.moveZDir(direction, deltaTime, speed);
}

void Scene::rotateCameraWithMouseInput(double xPos, double yPos)
{
	m_camera.rotateWithMouseInput(xPos, yPos);
}

void Scene::updateCameraLastMousePos(double x, double y)
{
	m_camera.updateLastMousePos(x, y);
}

tga::Buffer Scene::buffer() const
{
    return sceneBuffer;
}

const glm::mat4 &Scene::viewProjection() const
{
    return pSceneStagingBuffer->projectionView;
}

const DirLight &Scene::dirLight() const
{
    return pSceneStagingBuffer->dirLight;
}

const Camera &Scene::camera() const
{
    return m_camera;
}
