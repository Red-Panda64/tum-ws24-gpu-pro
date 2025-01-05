#include "Drawable.h"

Drawable::Drawable(tga::Interface &tgai, const Mesh &mesh, tga::RenderPass drawPass) : indexCount{mesh.indicesArray.size()}, tgai{&tgai} {
    size_t vb_size = mesh.verticesArray.size() * sizeof(tga::Vertex);
    size_t eb_size = mesh.indicesArray.size() * sizeof(int32_t);
    auto vbo_staging = tgai.createStagingBuffer({vb_size, reinterpret_cast<uint8_t*>(const_cast<tga::Vertex*>(mesh.verticesArray.data()))});
    vertexBuffer = tgai.createBuffer({tga::BufferUsage::vertex, vb_size, vbo_staging});
    tgai.free(vbo_staging);
    auto ebo_staging = tgai.createStagingBuffer({eb_size, reinterpret_cast<uint8_t*>(const_cast<uint32_t*>(mesh.indicesArray.data()))});
    indexBuffer = tgai.createBuffer({tga::BufferUsage::index, eb_size, ebo_staging});
    tgai.free(ebo_staging);
    meshInputSet = tgai.createInputSet({drawPass, { tga::Binding(mesh.diffuseTexture, 0, 0), tga::Binding(mesh.specularTexture, 1, 0) }, 1});
}

Drawable::~Drawable()
{
    tgai->free(meshInputSet);
    tgai->free(indexBuffer);
    tgai->free(vertexBuffer);
}

const tga::InputSet &Drawable::inputSet() const
{
    return meshInputSet;
}

void Drawable::draw(tga::CommandRecorder &recorder) const
{
    recorder.bindInputSet(meshInputSet);
    recorder.bindVertexBuffer(vertexBuffer);
    recorder.bindIndexBuffer(indexBuffer);
    recorder.drawIndexed(indexCount, 0, 0);
}
