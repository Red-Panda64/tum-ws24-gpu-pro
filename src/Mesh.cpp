#include "Mesh.h"
#include <filesystem>


tga::Texture loadTex(tga::Interface& tgai, const std::string& file, bool normalMap = false)
{
    int w, h, channels;
    uint8_t* p = stbi_load(file.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if(!p)
    {
        printf("Error while loading the texture on path: %s\n", file.c_str());
        return {};
    }

    tga::StagingBuffer textureStagingBuffer = tgai.createStagingBuffer({ 4 * (uint32_t)w * (uint32_t)h, p });
    tga::Format textureFormat;
    if(!normalMap)
    {
        textureFormat = tga::Format::r8g8b8a8_srgb;
    }
    else
    {
        textureFormat = tga::Format::r8g8b8a8_unorm;
    }
    tga::Texture texture = tgai.createTexture(tga::TextureInfo{ (uint32_t)w, (uint32_t)h, textureFormat, tga::SamplerMode::linear, tga::AddressMode::repeat }.setSrcData(textureStagingBuffer));
    free(p);
    tgai.free(textureStagingBuffer);
    return texture;
}

Mesh::Mesh(tga::Interface& tgai, const char* obj, const tga::VertexLayout& vertexLayout)
{
    std::filesystem::path objPath = obj;
    std::string texturesPath = objPath.replace_extension().string();
    std::string albedoTexturePath = texturesPath + std::string("_albedo.png");
    std::string normalTexturePath = texturesPath + std::string("_normal.png");
    std::string metallicTexturePath = texturesPath + std::string("_metal.png");
    std::string roughnessTexturePath = texturesPath + std::string("_roughness.png");
    std::string aoTextureMap = texturesPath + std::string("_ao.png");
    tga::Obj loadedObj = tga::loadObj(obj);
    verticesArray = loadedObj.vertexBuffer;
    indicesArray = loadedObj.indexBuffer;
    // Load the textures
    albedoMap = loadTex(tgai, albedoTexturePath);
    normalMap = loadTex(tgai, normalTexturePath, true);
    metallicMap = loadTex(tgai, metallicTexturePath);
    roughnessMap = loadTex(tgai, roughnessTexturePath);
    aoMap = loadTex(tgai, aoTextureMap);
}

tga::InputSet Mesh::getTextureInputSet(tga::Interface& tgai, const tga::RenderPass rp) const
{
    // Textures are on set 1 (hardcoded for the time being)
    return tgai.createInputSet({ rp, { tga::Binding(albedoMap, 0, 0), tga::Binding(normalMap, 1, 0), tga::Binding(metallicMap, 2, 0), tga::Binding(roughnessMap, 3, 0), tga::Binding(aoMap, 4, 0) }, 1 });
}

