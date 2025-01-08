#pragma once
#include "tga/tga.hpp"
#include "Mesh.h"

class Drawable {
public:
    Drawable(tga::Interface &tgai, const Mesh &mesh);
    ~Drawable();

    Drawable(const Drawable &other) = delete;
    Drawable &operator=(const Drawable &other) = delete;

    const tga::InputSet &inputSet() const;
    void draw(tga::CommandRecorder &recorder) const;

private:
    size_t indexCount;
    tga::Interface *tgai;
    tga::Buffer vertexBuffer;
    tga::Buffer indexBuffer;
};
