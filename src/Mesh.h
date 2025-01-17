#pragma once

#include <string>
#include <iostream>

#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"

struct ObjectUniformBuffer
{
    alignas(16) glm::mat4 model;
};

struct InstanceData
{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 rotation; // axis and angle
    alignas(16) float scale;
};

class Mesh
{
public:
	Mesh(tga::Interface& tgai, const char* objPath, const tga::VertexLayout& vertexLayout);

    tga::InputSet getTextureInputSet(tga::Interface& tgai, const tga::RenderPass rp) const;
public:
    std::vector<tga::Vertex> verticesArray;
    std::vector<uint32_t> indicesArray;
    tga::Texture albedoMap;
    tga::Texture normalMap;
    tga::Texture metallicMap;
    tga::Texture roughnessMap;
    tga::Texture aoMap;
};