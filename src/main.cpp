#include <chrono>
#include <thread>
#include <iostream>
#include <random>
#include <filesystem>
#include <unordered_map>
#include <cstdlib>
//#include <format>
#include <sstream>


#include "glm/gtc/type_ptr.hpp"
#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"

#include "Mesh.h"
#include "Scene.h"
#include "Drawable.h"
#include "ShadowPass.h"
#include "FogVolumeGenerationPass.h"

#define INSTANCE_COUNT 2048
#define PLACING_RADIUS 3000.0f
#define M_PI 3.14159265358979323846


tga::Interface tgai;
float aspectRatio;
int targetFPS = 144;

double getDeltaTime()
{
    typedef std::chrono::high_resolution_clock clock;
    typedef std::chrono::duration<double, std::milli> duration;

    static clock::time_point lastTime = clock::now();
    clock::time_point now = clock::now();
    duration delta = now - lastTime;
    lastTime = now;
    
    return delta.count();
}

void processInputs(const tga::Window& win, Scene& scene, double dt)
{
    float speed = 0.8;
    if(tgai.keyDown(win, tga::Key::Shift_Left))
    {
        speed *= 2;
    }

    bool cameraUpdated = false;
    // Camera Movement
    if(tgai.keyDown(win, tga::Key::W))
    {
        scene.moveCameraZDir(1, dt, speed);
        cameraUpdated = true;
    }
    if(tgai.keyDown(win, tga::Key::S))
    {
        scene.moveCameraZDir(-1, dt, speed);
        cameraUpdated = true;
    }
    if(tgai.keyDown(win, tga::Key::A))
    {
        scene.moveCameraXDir(-1, dt, speed);
        cameraUpdated = true;
    }
    if(tgai.keyDown(win, tga::Key::D))
    {
        scene.moveCameraXDir(1, dt, speed);
        cameraUpdated = true;
    }
    if(tgai.keyDown(win, tga::Key::Space))
    {
        scene.moveCameraYDir(1, dt, speed);
        cameraUpdated = true;
    }
    if(tgai.keyDown(win, tga::Key::Control_Left))
    {
        scene.moveCameraYDir(-1, dt, speed);
        cameraUpdated = true;
    }

    // Camera Rotation
    auto [xPos, yPos] = tgai.mousePosition(win);
    if(tgai.keyDown(win, tga::Key::MouseRight))
    {
        scene.rotateCameraWithMouseInput(xPos, yPos);
        cameraUpdated = true;
    }

    // Always update last mouse pos in camera
    scene.updateCameraLastMousePos(xPos, yPos);

    if(cameraUpdated)
    {
        scene.updateSceneBufferCameraData(aspectRatio);
    }

}

int main(int argc, const char *argv[])
{
    struct Flags {
        unsigned int changeDir : 1;
    } flags = {};

    auto usage = [argc, argv]() {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "./ex4") << " [-c] [<file>]\n";
        exit(1);
    };

    std::vector<std::string_view> positionalArgs;
    for(int argId = 1; argId < argc; argId++) {
        auto arg = std::string_view{argv[argId]};
        if(arg.empty()) {
            continue;
        }
        if(arg.at(0) != '-') {
            // Positional argument
            positionalArgs.push_back(arg);
        } else if(arg == "-c") {
            flags.changeDir = 1;
        } else {
            // Add more options here
            usage();
        }
    }
    std::filesystem::path meshDir;
    switch (positionalArgs.size()) {
        case 0:
            meshDir = "../assets";
            break;
        case 1:
            meshDir = positionalArgs[0];
            break;
        default:
            usage();
    }

    if (flags.changeDir && argc >= 0) {
        std::filesystem::path executable{argv[0]};
        std::filesystem::current_path(executable.parent_path());
    }

    // Window with the resolution of your screen
    auto [wWidth, wHeight] = tgai.screenResolution();
    auto win = tgai.createWindow({ wWidth, wHeight, tga::PresentMode::vsync });
    aspectRatio = (float)wWidth / wHeight;
    // Scene
    Scene scene(tgai);
    // Setup the camera
    scene.initCamera(glm::vec3(0.0f, 0.0f, 500.0f), glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)));
    scene.setAmbientFactor(0.05f);
    // Directional light
    scene.setDirLight(glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 1.0, 1.0f));
    static std::mt19937 rng(std::random_device{}());
    for(int i = 0; i < MAX_NR_OF_POINT_LIGHTS; ++i)
    {
        std::uniform_real_distribution<float> posDist(-1250, 1250);
        std::uniform_real_distribution<float> colorDist(0, 100);
        scene.addPointLight(glm::vec3(posDist(rng), posDist(rng), posDist(rng)), glm::vec3(colorDist(rng), colorDist(rng), colorDist(rng)), glm::vec3(1.0f, 0.007f, 0.0002f));
    }


    // Update Camera Data at the beginning
    scene.updateSceneBufferCameraData(aspectRatio);

    // Prepare the vertex layout for the meshes (all the obj loaded meshes share the same structure)
    tga::VertexLayout vertexLayout(
        sizeof(tga::Vertex),
        {
            {offsetof(tga::Vertex, position), tga::Format::r32g32b32_sfloat},
            {offsetof(tga::Vertex, uv), tga::Format::r32g32_sfloat},
            {offsetof(tga::Vertex, normal), tga::Format::r32g32b32_sfloat},
            {offsetof(tga::Vertex, tangent), tga::Format::r32g32b32_sfloat},
        }
    );
    
    // Prepare the Input (whole collection of sets) Layout (Descriptor Set(s))
    // Set 0: Global Scene Data, Set 1: mesh data, Set 2: object data
    tga::SetLayout meshDescriptorSet0Layout = tga::SetLayout{ {tga::BindingType::uniformBuffer, tga::BindingType::uniformBuffer, tga::BindingType::sampler, tga::BindingType::sampler, tga::BindingType::uniformBuffer} };
    tga::SetLayout meshDescriptorSet1Layout = tga::SetLayout{ {tga::BindingType::sampler, tga::BindingType::sampler} };
    tga::SetLayout meshDescriptorSet2Layout = tga::SetLayout{ {tga::BindingType::uniformBuffer} };
    tga::InputLayout meshDescriptorLayout = tga::InputLayout( { meshDescriptorSet0Layout, meshDescriptorSet1Layout, meshDescriptorSet2Layout } );

    // Load shader code from file
    auto vs = tga::loadShader("../shaders/mesh_vert.spv", tga::ShaderType::vertex, tgai);
    auto fs = tga::loadShader("../shaders/mesh_frag.spv", tga::ShaderType::fragment, tgai);

    //auto cs = tga::loadShader("../shaders/compute_transforms_comp.spv", tga::ShaderType::compute, tgai);
    //tga::SetLayout computeSetLayout = tga::SetLayout{ {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::uniformBuffer} };
    //tga::InputLayout computeInputLayout = tga::InputLayout({ computeSetLayout });
    //auto cp = tgai.createComputePass({cs, computeInputLayout});

    Mesh windowMesh{tgai, "../assets/window.obj", vertexLayout};
    glm::mat4 windowTransform = glm::mat4(1.0);
    windowTransform = glm::scale(windowTransform, glm::vec3(5.0, 5.0, 5.0));
    // TODO: use single buffer
    tga::StagingBuffer windowStagingBuffer = tgai.createStagingBuffer({sizeof(glm::mat4), reinterpret_cast<uint8_t*>(glm::value_ptr(windowTransform))});
    tga::Buffer windowTransformBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(glm::mat4), windowStagingBuffer, 0 });
    Mesh planeMesh{tgai, "../assets/plane.obj", vertexLayout};
    glm::mat4 planeTransform = glm::mat4(1.0);
    planeTransform = glm::scale(planeTransform, glm::vec3(100.0, 100.0, 100.0));
    tga::StagingBuffer planeStagingBuffer = tgai.createStagingBuffer({sizeof(glm::mat4), reinterpret_cast<uint8_t*>(glm::value_ptr(planeTransform))});
    tga::Buffer planeTransformBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(glm::mat4), planeStagingBuffer, 0 });
    
    constexpr uint32_t SHADOW_MAP_RESX = 4096;
    constexpr uint32_t SHADOW_MAP_RESY = 4096;
    ShadowPass sp{ tgai, { SHADOW_MAP_RESX, SHADOW_MAP_RESY }, vertexLayout };
    FogVolumeGenerationPass fp {tgai, { 512, 256, 128 }, sp};

    // Create the Render pass
    auto rpInfo = tga::RenderPassInfo{vs, fs, win}
        .setClearOperations(tga::ClearOperation::all)
        .setPerPixelOperations(tga::PerPixelOperations{}.setDepthCompareOp(tga::CompareOperation::lessEqual))
        .setRasterizerConfig(tga::RasterizerConfig().setFrontFace(tga::FrontFace::counterclockwise).setCullMode(tga::CullMode::back))
        .setInputLayout(meshDescriptorLayout)
        .setVertexLayout(vertexLayout);
    auto rp = tgai.createRenderPass(rpInfo);
    Drawable windowDrawable{tgai, windowMesh};
    tga::InputSet windowInputSet = tgai.createInputSet({ rp, { tga::Binding(windowMesh.diffuseTexture, 0, 0), tga::Binding(windowMesh.specularTexture, 1, 0) }, 1 });
    tga::InputSet windowTransformInputSet = tgai.createInputSet({rp, { tga::Binding(windowTransformBuffer, 0, 0) }, 2});
    tga::InputSet windowTransformShadowInputSet = tgai.createInputSet({sp.renderPass(), { tga::Binding(windowTransformBuffer, 0, 0) }, 1});
    Drawable planeDrawable{tgai, planeMesh};
    tga::InputSet planeInputSet = tgai.createInputSet({ rp, { tga::Binding(planeMesh.diffuseTexture, 0, 0), tga::Binding(planeMesh.specularTexture, 1, 0) }, 1 });
    tga::InputSet planeTransformInputSet = tgai.createInputSet({rp, { tga::Binding(planeTransformBuffer, 0, 0) }, 2});
    tga::InputSet planeTransformShadowInputSet = tgai.createInputSet({sp.renderPass(), { tga::Binding(planeTransformBuffer, 0, 0) }, 1});

    // Create global input (descriptor) set
    tga::InputSet globalInput = tgai.createInputSet({ rp, { tga::Binding(scene.buffer(), 0), tga::Binding(sp.inputBuffer(), 1), tga::Binding(sp.shadowMap(), 2), tga::Binding(fp.scatteringVolume(), 3), tga::Binding(fp.inputBuffer(), 4) } , 0 });

    std::vector<tga::CommandBuffer> cmdBuffers(tgai.backbufferCount(win));
    auto rebuildCmdBuffers = [&]() {
        // Prepare the command buffers
        for(size_t i = 0; i < cmdBuffers.size(); ++i)
        {
            tgai.free(cmdBuffers[i]);
            cmdBuffers[i] = {};
            tga::CommandRecorder recorder = tga::CommandRecorder{ tgai, cmdBuffers[i] };
            // Scene Buffer is global and every mesh using the pipeline (we only have 1) uses the same buffer so loading it once per frame.
            scene.bufferUpload(recorder);
            recorder.bufferUpload(windowStagingBuffer, windowTransformBuffer, sizeof(glm::mat4));
            recorder.bufferUpload(planeStagingBuffer, planeTransformBuffer, sizeof(glm::mat4));

            sp.upload(recorder);
            fp.upload(recorder);

            recorder.barrier(tga::PipelineStage::Transfer, tga::PipelineStage::VertexShader);

            // Shadow pass
            sp.bind(recorder, i);
            recorder.bindInputSet(windowTransformShadowInputSet);
            windowDrawable.draw(recorder);
            recorder.bindInputSet(planeTransformShadowInputSet);
            planeDrawable.draw(recorder);

            // Volume compute pass
            recorder.setRenderPass(tga::RenderPass{nullptr}, i);
            recorder.barrier(tga::PipelineStage::ColorAttachmentOutput, tga::PipelineStage::ComputeShader);
            fp.execute(recorder, i);
            recorder.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::FragmentShader);

            // Forward pass
            recorder.setRenderPass(rp, i, {0.0, 0.0, 0.0, 1.0});
            recorder.bindInputSet(globalInput);
            recorder.bindInputSet(windowInputSet);
            recorder.bindInputSet(windowTransformInputSet);
            windowDrawable.draw(recorder);
            recorder.bindInputSet(planeInputSet);
            recorder.bindInputSet(planeTransformInputSet);
            planeDrawable.draw(recorder);

            cmdBuffers[i] = recorder.endRecording();
        }
    };

    rebuildCmdBuffers();

    // Frame limit parameters
    double targetFrequency = (1.0 / targetFPS) * 1000; // in ms
    std::chrono::system_clock::time_point currentFrameStart;
    std::chrono::system_clock::time_point prevFrameEnd;
    double smoothedFps = 0.0;
    constexpr float historyWeight = 0.9;
    // Record and present until signal to close
    double time = 0.0;
    while (!tgai.windowShouldClose(win)) 
    {
        // FPS LIMITING
        // Maintain designated frequency of 1 / targetFPS
        currentFrameStart = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> work_time = currentFrameStart - prevFrameEnd;

        if(work_time.count() < targetFrequency)
        {
            std::chrono::duration<double, std::milli> delta_ms(targetFrequency - work_time.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
        }
        prevFrameEnd = std::chrono::system_clock::now();

        double dt = getDeltaTime();
        time += dt;
        bool shouldRebuildCmdBuffers = false;
        if(shouldRebuildCmdBuffers) {
            rebuildCmdBuffers();
        }
        double fps = 1000.0f / dt;
        smoothedFps = fps * (1.0 - historyWeight) + smoothedFps * historyWeight;
        std::stringstream sstream;
        sstream.setf(std::ios::fixed, std::ios::floatfield);
        sstream.precision(3);
        sstream << "[FPS]: " << fps << " (Smoothed: " << smoothedFps << ")";
        tgai.setWindowTitle(win, sstream.str());//std::format("[FPS]: {} (Smoothed: {})", fps, smoothedFps));
        processInputs(win, scene, dt);
        sp.update(scene, 0.001f, 0.04f);
        auto nf = tgai.nextFrame(win);
        fp.update(scene, nf);
        auto& cmd = cmdBuffers[nf];
        tgai.execute(cmd);
        tgai.present(win, nf);
        tgai.waitForCompletion(cmd);
    }

    return 0;
}
