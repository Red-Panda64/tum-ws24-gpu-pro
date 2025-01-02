#include <chrono>
#include <thread>
#include <iostream>
#include <random>
#include <filesystem>
#include <unordered_map>
#include <cstdlib>
//#include <format>
#include <sstream>


#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"

#include "Mesh.h"
#include "Scene.h"

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
    scene.setDirLight(glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0, 1.0f));
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

    constexpr size_t batchSize = 512;
    
    // Prepare the Input (whole collection of sets) Layout (Descriptor Set(s))
    // Set 0: Global Scene Data, Set 1: Textures for all the meshes (2 textures per mesh) Set3: Per Instance Transform Data (This is a storage buffer not a uniform buffer)
    tga::SetLayout meshDescriptorSet0Layout = tga::SetLayout{ {tga::BindingType::uniformBuffer} };
    tga::SetLayout meshDescriptorSet1Layout = tga::SetLayout{ {tga::BindingType::sampler, static_cast<uint32_t>(batchSize * 2)} };
    tga::SetLayout meshDescriptorSet2Layout = tga::SetLayout{ {tga::BindingType::storageBuffer} };
    tga::InputLayout meshDescriptorLayout = tga::InputLayout({ meshDescriptorSet0Layout, meshDescriptorSet1Layout, meshDescriptorSet2Layout});

    // Load shader code from file
    auto vs = tga::loadShader("../shaders/mesh_multidraw_vert.spv", tga::ShaderType::vertex, tgai);
    auto fs = tga::loadShader("../shaders/mesh_multidraw_frag.spv", tga::ShaderType::fragment, tgai);
    auto cs = tga::loadShader("../shaders/compute_transforms_comp.spv", tga::ShaderType::compute, tgai);

    tga::SetLayout computeSetLayout = tga::SetLayout{ {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::uniformBuffer} };
    tga::InputLayout computeInputLayout = tga::InputLayout({ computeSetLayout });

    auto cp = tgai.createComputePass({cs, computeInputLayout});

    // Create the Render pass
    auto rpInfo = tga::RenderPassInfo{vs, fs, win}
        .setClearOperations(tga::ClearOperation::all)
        .setPerPixelOperations(tga::PerPixelOperations{}.setDepthCompareOp(tga::CompareOperation::lessEqual))
        .setRasterizerConfig(tga::RasterizerConfig().setFrontFace(tga::FrontFace::counterclockwise).setCullMode(tga::CullMode::back))
        .setInputLayout({ meshDescriptorLayout })
        .setVertexLayout(vertexLayout);

    auto rp = tgai.createRenderPass(rpInfo);

    // Create scene (global) input (descriptor) set
    scene.createSceneInputSet(tgai, rp);

    std::vector<tga::CommandBuffer> cmdBuffers(tgai.backbufferCount(win));
    auto rebuildCmdBuffers = [&]() {
        // Prepare the command buffers
        for(int i = 0; i < cmdBuffers.size(); ++i)
        {
            tgai.free(cmdBuffers[i]);
            cmdBuffers[i] = {};
            tga::CommandRecorder recorder = tga::CommandRecorder{ tgai, cmdBuffers[i] };
            // Scene Buffer is global and every mesh using the pipeline (we only have 1) uses the same buffer so loading it once per frame.
            scene.bufferUpload(recorder);

            //TODO: upload data

            recorder.barrier(tga::PipelineStage::Transfer, tga::PipelineStage::VertexShader);

            recorder.setRenderPass(rp, i);
            scene.bindSceneInputSet(recorder);

            //TODO: commands for drawing

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
        auto nf = tgai.nextFrame(win);
        auto& cmd = cmdBuffers[nf];
        tgai.execute(cmd);
        tgai.present(win, nf);
        tgai.waitForCompletion(cmd);
    }

    return 0;
}
