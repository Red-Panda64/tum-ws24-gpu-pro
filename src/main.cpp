#include <chrono>
#include <thread>
#include <iostream>
#include <random>
#include <filesystem>
#include <unordered_map>
#include <cstdlib>
//#include <format>
#include <sstream>
#include <algorithm>


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
glm::uvec2 viewport;
int targetFPS = 144;
float historyFactor = 0.85f;

class BindingSetInstance;

class BindingSetDescription {
private:
    friend class BindingSetInstance;
    size_t set;
    std::vector<tga::Binding> fixedBindings;
    std::unordered_map<std::string, std::pair<size_t, size_t>> namedBindings;
public:
    BindingSetDescription(size_t set) : set{set} {}

    BindingSetDescription &fix(tga::Binding binding) {
        fixedBindings.push_back(binding);
        return *this;
    }

    BindingSetDescription &declare(std::string name, size_t bindingLoc, size_t arraySlot) {
        namedBindings.emplace(std::move(name), std::make_pair<size_t, size_t>(std::move(bindingLoc), std::move(arraySlot)));
        return *this;
    }
};

class BindingSetInstance {
public:
    BindingSetInstance(const BindingSetDescription &desc) : bindings{desc.fixedBindings}, desc{desc} {}

    BindingSetInstance &assign(const std::string &name, tga::Binding::Resource resource) {
        // for heterogenous lookup (std::string_view as permissible argument instead of std::string), switch to C++23
        auto it = desc.namedBindings.find(name);
        if(it != desc.namedBindings.end()) {
            bindings.push_back(tga::Binding(resource, it->second.first, it->second.second));
        }
        return *this;
    }

    tga::InputSet build(tga::Interface &tgai, tga::RenderPass rp) {
        return tgai.createInputSet({ rp, bindings, desc.set }); 
    };

private:
    std::vector<tga::Binding> bindings;
    const BindingSetDescription &desc;
};

struct HashRenderpass {
    size_t operator()(const tga::RenderPass &rp) const {
        TgaRenderPass tgarp = rp;
        return std::hash<TgaRenderPass>{}(tgarp);
    }
};

template<typename T>
using PerRP = std::unordered_map<tga::RenderPass, T, HashRenderpass>;

struct MeshTable {
public:
    MeshTable() = default;
    ~MeshTable() {
        for (auto &[_, inputSets] : mtoTextures) {
            for(auto &[_, inputSet] : inputSets) {
                tgai.free(inputSet);
            }
        }
    }
    MeshTable(const MeshTable &other) = delete;
    MeshTable &operator=(const MeshTable &other) = delete;

    void registerPass(tga::RenderPass rp, BindingSetDescription bDesc) {
        registeredPasses.emplace_back(rp, std::move(bDesc));
        const BindingSetDescription &bDescRef = registeredPasses.back().second;
        for(auto &[meshTag, mesh] : registeredMeshes) {
            createInputSet(meshTag, mesh, rp, bDescRef);
        } 
    }

    void load(std::string meshTag) {
        if(mtoD.find(meshTag) != mtoD.end())
            return;
        tga::VertexLayout vertexLayout(
            sizeof(tga::Vertex),
            {
                {offsetof(tga::Vertex, position), tga::Format::r32g32b32_sfloat},
                {offsetof(tga::Vertex, uv), tga::Format::r32g32_sfloat},
                {offsetof(tga::Vertex, normal), tga::Format::r32g32b32_sfloat},
                {offsetof(tga::Vertex, tangent), tga::Format::r32g32b32_sfloat},
            }
        );
        Mesh mesh{tgai, ("../assets/" + meshTag + "/" + meshTag + ".obj").c_str(), vertexLayout};
        mtoD.emplace(std::piecewise_construct,
              std::forward_as_tuple(meshTag),
              std::forward_as_tuple(tgai, mesh));
        for(auto &[rp, bDesc] : registeredPasses) {
            createInputSet(meshTag, mesh, rp, bDesc);
        }

        registeredMeshes.emplace_back(std::move(meshTag), std::move(mesh));
    }

    std::vector<std::pair<std::string, Mesh>> registeredMeshes;
    std::unordered_map<std::string, Drawable> mtoD;
    std::unordered_map<std::string, PerRP<tga::InputSet>> mtoTextures;
    std::vector<std::pair<tga::RenderPass, BindingSetDescription>> registeredPasses;
private:
    void createInputSet(const std::string &meshTag, const Mesh &mesh, tga::RenderPass rp, const BindingSetDescription &bDesc) {
        mtoTextures[meshTag][rp] = BindingSetInstance{bDesc}
            .assign("albedo", mesh.albedoMap)
            .assign("normal", mesh.normalMap)
            .assign("metallic", mesh.metallicMap)
            .assign("roughness", mesh.roughnessMap)
            .assign("ao", mesh.aoMap).build(tgai, rp);
    }
} meshTable;

class Demo {
public:
    Demo() = default; 
    virtual ~Demo() {
        for(auto &[staging, _0, _1] : uploads) {
            static_cast<void>(_0); static_cast<void>(_1);
            tgai.free(staging);
        }
        for(auto &[_0, buffers] : mtoTransformBuffers) {
            static_cast<void>(_0);
            for(auto buffer : buffers) {
                tgai.free(buffer);
            }
        }
        for(auto &[_0, instanceInputSets] : mtoTransforms) {
            static_cast<void>(_0);
            for(auto inputSets : instanceInputSets) {
                for(auto [_1, inputSet] : inputSets) {
                    static_cast<void>(_1);
                    tgai.free(inputSet);
                }
            }
        }
    };
    Demo(const Demo &other) = delete;
    Demo &operator=(const Demo &other) = delete;

    virtual void update(float dt) = 0;

    std::unordered_map<std::string, std::vector<PerRP<tga::InputSet>>> mtoTransforms;
    std::unordered_map<std::string, std::vector<tga::Buffer>> mtoTransformBuffers;
    std::vector<std::tuple<tga::StagingBuffer, tga::Buffer, size_t>> uploads;
    std::vector<std::pair<tga::RenderPass, BindingSetDescription>> registeredPasses;

    void registerPass(tga::RenderPass rp, BindingSetDescription bDesc) {
        registeredPasses.emplace_back(rp, std::move(bDesc));
        for(auto &[meshTag, instanceInputSets] : mtoTransforms) {
            const std::vector<tga::Buffer> &tbs = mtoTransformBuffers.at(meshTag);
            for(size_t i = 0, end = instanceInputSets.size(); i < end; ++i) {
                createInputSet(instanceInputSets[i], tbs[i], rp, registeredPasses.back().second);
            }
        }
    }

protected:
    size_t addInstance(std::string name, const glm::mat4 &transform) {
        meshTable.load(name);
        size_t idx = uploads.size();
        tga::StagingBuffer sb = tgai.createStagingBuffer({ sizeof(glm::mat4), reinterpret_cast<const uint8_t*>(glm::value_ptr(transform)) });
        tga::Buffer tb = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(glm::mat4), sb });
        uploads.push_back({ sb, tb, sizeof(glm::mat4) });
        mtoTransformBuffers[name].push_back(tb);
        auto &inputSets = mtoTransforms[name];
        inputSets.push_back(PerRP<tga::InputSet>{});
        for(auto &[rp, bDesc] : registeredPasses) {
            inputSets.back().emplace(rp, BindingSetInstance{bDesc}.assign("transform", tb).build(tgai, rp));
        }
        return idx;
    }
private:
    void createInputSet(PerRP<tga::InputSet> &inputSets, tga::Buffer tb, tga::RenderPass rp, const BindingSetDescription &bDesc) {
        inputSets.emplace(rp, BindingSetInstance{bDesc}.assign("transform", tb).build(tgai, rp));
    }
};

std::vector<std::unique_ptr<Demo>> demos;
Demo *currentDemo;

class CitadelDemo : public Demo {
public:
    CitadelDemo() {
        addInstance("window", glm::mat4(1.0f));
        addInstance("plane", glm::mat4(100.0f));
    }

    void update(float dt) { static_cast<void>(dt); }
};

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
    float speed = 0.01;
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

    if(tgai.keyDown(win, tga::Key::Enter))
    {
        historyFactor = 1.0f;
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
        scene.updateSceneBufferCameraData(viewport);
    }

}

void setupDemos(tga::Interface &tgai) {
    demos.emplace_back(std::make_unique<CitadelDemo>());
    currentDemo = demos.back().get();
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
    viewport = glm::uvec2(wWidth, wHeight);
    // Scene
    Scene scene(tgai);
    // Setup the camera
    scene.initCamera(glm::vec3(0.0f, 10.0f, 10.0f), 0.0f, 0.0f, glm::radians(180.0f));
    scene.setAmbientFactor(0.03f);
    // Directional light
    scene.setDirLight(glm::normalize(glm::vec3(10.0f, -10.0f, -1.0f)), glm::vec3(1.0f));
    static std::mt19937 rng(std::random_device{}());
    for(int i = 0; i < MAX_NR_OF_POINT_LIGHTS; ++i)
    {
        std::uniform_real_distribution<float> posDist(-50, 50);
        // std::uniform_real_distribution<float> colorDist(0, 100);
        scene.addPointLight(glm::vec3(posDist(rng), posDist(rng), posDist(rng)), glm::vec3(150.0f), glm::vec3(1.0f, 0.007f, 0.0002f));
    }

    // Update Camera Data at the beginning
    scene.updateSceneBufferCameraData(viewport);

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
    tga::SetLayout meshDescriptorSet1Layout = tga::SetLayout{ {tga::BindingType::sampler, tga::BindingType::sampler, tga::BindingType::sampler, tga::BindingType::sampler, tga::BindingType::sampler} };
    tga::SetLayout meshDescriptorSet2Layout = tga::SetLayout{ {tga::BindingType::uniformBuffer} };
    tga::InputLayout meshDescriptorLayout = tga::InputLayout( { meshDescriptorSet0Layout, meshDescriptorSet1Layout, meshDescriptorSet2Layout } );

    // Load shader code from file
    auto vs = tga::loadShader("../shaders/mesh_vert.spv", tga::ShaderType::vertex, tgai);
    auto fs = tga::loadShader("../shaders/mesh_frag.spv", tga::ShaderType::fragment, tgai);

    //auto cs = tga::loadShader("../shaders/compute_transforms_comp.spv", tga::ShaderType::compute, tgai);
    //tga::SetLayout computeSetLayout = tga::SetLayout{ {tga::BindingType::storageBuffer}, {tga::BindingType::storageBuffer}, {tga::BindingType::uniformBuffer} };
    //tga::InputLayout computeInputLayout = tga::InputLayout({ computeSetLayout });
    //auto cp = tgai.createComputePass({cs, computeInputLayout});

    //Mesh windowMesh{tgai, "../assets/window.obj", vertexLayout};
    //glm::mat4 windowTransform = glm::mat4(1.0);
    //windowTransform = glm::scale(windowTransform, glm::vec3(5.0, 5.0, 5.0));
    //// TODO: use single buffer
    //tga::StagingBuffer windowStagingBuffer = tgai.createStagingBuffer({sizeof(glm::mat4), reinterpret_cast<uint8_t*>(glm::value_ptr(windowTransform))});
    //tga::Buffer windowTransformBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(glm::mat4), windowStagingBuffer, 0 });
    
    meshTable.load("plane");
    glm::mat4 planeTransform = glm::mat4(1.0);
    planeTransform = glm::scale(planeTransform, glm::vec3(100.0, 100.0, 100.0));
    tga::StagingBuffer planeStagingBuffer = tgai.createStagingBuffer({sizeof(glm::mat4), reinterpret_cast<uint8_t*>(glm::value_ptr(planeTransform))});
    tga::Buffer planeTransformBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(glm::mat4), planeStagingBuffer, 0 });

    meshTable.load("brass");
    glm::mat4 pbrMeshTransform = glm::mat4(1.0);
    pbrMeshTransform = glm::scale(pbrMeshTransform, glm::vec3(20.0, 20.0, 20.0));
    tga::StagingBuffer pbrMeshStagingBuffer = tgai.createStagingBuffer({sizeof(glm::mat4), reinterpret_cast<uint8_t*>(glm::value_ptr(pbrMeshTransform))});
    tga::Buffer pbrMeshTransformBuffer = tgai.createBuffer({ tga::BufferUsage::uniform, sizeof(glm::mat4), pbrMeshStagingBuffer, 0 });

    constexpr uint32_t SHADOW_MAP_RESX = 4096;
    constexpr uint32_t SHADOW_MAP_RESY = 4096;
    ShadowPass sp{ tgai, { SHADOW_MAP_RESX, SHADOW_MAP_RESY }, vertexLayout };
    FogVolumeGenerationPass fp {tgai, { 256, 256, 256 }, sp};

    // Create the Render pass
    auto rpInfo = tga::RenderPassInfo{vs, fs, win}
        .setClearOperations(tga::ClearOperation::all)
        .setPerPixelOperations(tga::PerPixelOperations{}.setDepthCompareOp(tga::CompareOperation::lessEqual))
        .setRasterizerConfig(tga::RasterizerConfig().setFrontFace(tga::FrontFace::counterclockwise).setCullMode(tga::CullMode::back))
        .setInputLayout(meshDescriptorLayout)
        .setVertexLayout(vertexLayout);
    auto rp = tgai.createRenderPass(rpInfo);

    setupDemos(tgai);
    meshTable.registerPass(rp, std::move(BindingSetDescription{1}.declare("albedo", 0, 0).declare("normal", 1, 0).declare("metallic", 2, 0).declare("roughness", 3, 0).declare("ao", 4, 0)));
    for(auto &demo : demos) {
        demo->registerPass(rp, std::move(BindingSetDescription{2}.declare("transform", 0, 0)));
        demo->registerPass(sp.renderPass(), std::move(BindingSetDescription{1}.declare("transform", 0, 0)));
    }

    // Load shader code from file
    auto skyVs = tga::loadShader("../shaders/sky_vert.spv", tga::ShaderType::vertex, tgai);
    auto skyFs = tga::loadShader("../shaders/sky_frag.spv", tga::ShaderType::fragment, tgai);
    auto skyRpInfo = tga::RenderPassInfo{skyVs, skyFs, win}
        .setClearOperations(tga::ClearOperation::none)
        .setPerPixelOperations(tga::PerPixelOperations{}.setDepthCompareOp(tga::CompareOperation::lessEqual))
        .setRasterizerConfig(tga::RasterizerConfig().setFrontFace(tga::FrontFace::counterclockwise).setCullMode(tga::CullMode::back))
        .setInputLayout(tga::InputLayout( { tga::SetLayout { tga::BindingType::uniformBuffer, tga::BindingType::sampler, tga::BindingType::uniformBuffer } } ))
        .setVertexLayout(tga::VertexLayout { 0, {} });
    auto skyRp = tgai.createRenderPass(skyRpInfo);
    tga::InputSet skyInput = tgai.createInputSet({ skyRp, { tga::Binding(scene.buffer(), 0), tga::Binding(fp.scatteringVolume(), 1), tga::Binding(fp.inputBuffer(), 2) } , 0 });

    // Create global input (descriptor) set
    tga::InputSet globalInput = tgai.createInputSet({ rp, { tga::Binding(scene.buffer(), 0), tga::Binding(sp.inputBuffer(), 1), tga::Binding(sp.shadowMap(), 2), tga::Binding(fp.scatteringVolume(), 3), tga::Binding(fp.inputBuffer(), 4) } , 0 });

    std::vector<tga::CommandBuffer> cmdBuffers(tgai.backbufferCount(win));

    auto renderMeshes = [](tga::CommandRecorder &recorder, tga::RenderPass rp) {
        for(auto [meshName, transforms] : currentDemo->mtoTransforms) {
            auto textureSets = meshTable.mtoTextures.at(meshName);
            auto textureSetIt = textureSets.find(rp);
            if(textureSetIt != textureSets.end()) {
                recorder.bindInputSet(textureSetIt->second);
            } // else no textures needed for this pass
            Drawable &drawable = meshTable.mtoD.at(meshName);
            for(auto &transform : transforms) {
                recorder.bindInputSet(transform.at(rp));
                drawable.draw(recorder);
            }
        }
    };
    
    auto rebuildCmdBuffers = [&]() {
        // Prepare the command buffers
        for(size_t i = 0; i < cmdBuffers.size(); ++i)
        {
            tgai.free(cmdBuffers[i]);
            cmdBuffers[i] = {};
            tga::CommandRecorder recorder = tga::CommandRecorder{ tgai, cmdBuffers[i] };
            // Scene Buffer is global and every mesh using the pipeline (we only have 1) uses the same buffer so loading it once per frame.
            scene.bufferUpload(recorder);
            for(auto [staging, target, size] : currentDemo->uploads) {
                recorder.bufferUpload(staging, target, size);
            }

            sp.upload(recorder);
            fp.upload(recorder);

            recorder.barrier(tga::PipelineStage::Transfer, tga::PipelineStage::VertexShader);

            // Shadow pass
            sp.bind(recorder, i);
            renderMeshes(recorder, sp.renderPass());

            // Volume compute pass
            recorder.setRenderPass(tga::RenderPass{nullptr}, i);
            recorder.barrier(tga::PipelineStage::ColorAttachmentOutput, tga::PipelineStage::ComputeShader);
            fp.execute(recorder, i);
            recorder.barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::FragmentShader);

            // Forward pass
            recorder.setRenderPass(rp, i, {0.0, 0.0, 0.0, 1.0});
            recorder.bindInputSet(globalInput);
            renderMeshes(recorder, rp);

            recorder.setRenderPass(skyRp, i);
            recorder.bindInputSet(skyInput);
            recorder.draw(6, 0);

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
    uint64_t frameNumber = 0;
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
        fp.update(scene, frameNumber++, historyFactor);
        auto nf = tgai.nextFrame(win);
        auto& cmd = cmdBuffers[nf];
        tgai.execute(cmd);
        tgai.present(win, nf);
        tgai.waitForCompletion(cmd);
    }

    return 0;
}
