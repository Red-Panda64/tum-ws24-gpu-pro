#include "Drawable.h"

Drawable::Drawable(tga::Interface &tgai, const Mesh &mesh) : indexCount{mesh.indicesArray.size()}, tgai{&tgai} {
    size_t vb_size = mesh.verticesArray.size() * sizeof(tga::Vertex);
    size_t eb_size = mesh.indicesArray.size() * sizeof(int32_t);
    auto vbo_staging = tgai.createStagingBuffer({vb_size, reinterpret_cast<uint8_t*>(const_cast<tga::Vertex*>(mesh.verticesArray.data()))});
    vertexBuffer = tgai.createBuffer({tga::BufferUsage::vertex, vb_size, vbo_staging});
    tgai.free(vbo_staging);
    auto ebo_staging = tgai.createStagingBuffer({eb_size, reinterpret_cast<uint8_t*>(const_cast<uint32_t*>(mesh.indicesArray.data()))});
    indexBuffer = tgai.createBuffer({tga::BufferUsage::index, eb_size, ebo_staging});
    tgai.free(ebo_staging);
}

Drawable::~Drawable()
{
    tgai->free(indexBuffer);
    tgai->free(vertexBuffer);
}

void Drawable::draw(tga::CommandRecorder &recorder) const
{
    recorder.bindVertexBuffer(vertexBuffer);
    recorder.bindIndexBuffer(indexBuffer);
    recorder.drawIndexed(indexCount, 0, 0);
}
