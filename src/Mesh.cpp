#include "Mesh.h"
#include <filesystem>


tga::Texture loadTex(tga::Interface& tgai, const std::string& file)
{
    int w, h, channels;
    uint8_t* p = stbi_load(file.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if(!p)
    {
        printf("Error while loading the texture on path: %s\n", file.c_str());
        return {};
    }

    tga::StagingBuffer textureStagingBuffer = tgai.createStagingBuffer({ 4 * (uint32_t)w * (uint32_t)h, p });
    tga::Texture texture = tgai.createTexture(tga::TextureInfo{ (uint32_t)w, (uint32_t)h, tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear, tga::AddressMode::repeat }.setSrcData(textureStagingBuffer));
    free(p);
    tgai.free(textureStagingBuffer);
    return texture;
}

Mesh::Mesh(tga::Interface& tgai, const char* obj, const tga::VertexLayout& vertexLayout)
{
    std::filesystem::path objPath = obj;
    std::string diffuseTexturePath = objPath.replace_extension().string();
    std::string specularTexturePath = diffuseTexturePath;
    diffuseTexturePath.append("_diffuse.png");
    specularTexturePath.append("_specular.png");
    tga::Obj loadedObj = tga::loadObj(obj);
    verticesArray = loadedObj.vertexBuffer;
    indicesArray = loadedObj.indexBuffer;
    // Load the textures
    diffuseTexture = loadTex(tgai, diffuseTexturePath);
    specularTexture = loadTex(tgai, specularTexturePath);
}

