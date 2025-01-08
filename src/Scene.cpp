#include "Scene.h"

Scene::Scene(tga::Interface& tgai)
{
	prepareSceneUniformBuffer(tgai);
}

void Scene::initCamera(const glm::vec3& pos, const glm::quat& orientation)
{
	camera = Camera(pos, orientation);
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
	new(pSceneStagingBuffer) SceneUniformBuffer { .nrPointLights = 0 };
	// The actual buffer in GPU
	sceneBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(SceneUniformBuffer), sceneStagingBuffer });
}

void Scene::updateSceneBufferCameraData(float aspectRatio)
{
	pSceneStagingBuffer->projectionView = camera.projection(aspectRatio) * camera.view();
	pSceneStagingBuffer->cameraPos = camera.getPosition();
}

void Scene::createSceneInputSet(tga::Interface& tgai, tga::RenderPass& rp)
{
	sceneInputSet = tgai.createInputSet({ rp, {tga::Binding(sceneBuffer, 0)} , 0 });
}

void Scene::bufferUpload(tga::CommandRecorder& recorder)
{
	recorder.bufferUpload(sceneStagingBuffer, sceneBuffer, sizeof(SceneUniformBuffer));
}

void Scene::bindSceneInputSet(tga::CommandRecorder& recorder)
{
	recorder.bindInputSet(sceneInputSet);
}

void Scene::moveCamera(const glm::vec3& direction, float deltaTime, float speed)
{
	camera.move(direction, deltaTime, speed);
}

void Scene::moveCameraXDir(float direction, float deltaTime, float speed)
{
	camera.moveXDir(direction, deltaTime, speed);
}

void Scene::moveCameraYDir(float direction, float deltaTime, float speed)
{
	camera.moveYDir(direction, deltaTime, speed);
}

void Scene::moveCameraZDir(float direction, float deltaTime, float speed)
{
	camera.moveZDir(direction, deltaTime, speed);
}

void Scene::rotateCameraWithMouseInput(double xPos, double yPos)
{
	camera.rotateWithMouseInput(xPos, yPos);
}

void Scene::updateCameraLastMousePos(double x, double y)
{
	camera.updateLastMousePos(x, y);
}

const glm::mat4 &Scene::viewProjection() const
{
    return pSceneStagingBuffer->projectionView;
}

const DirLight &Scene::dirLight() const
{
    return pSceneStagingBuffer->dirLight;
}
