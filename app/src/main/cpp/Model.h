#ifndef ANDROIDGLINVESTIGATIONS_MODEL_H
#define ANDROIDGLINVESTIGATIONS_MODEL_H

#include <vector>
#include "TextureAsset.h"
#include <glm/glm.hpp>

class Model {
public:
    inline Model(
            GLuint vertexArray,
            GLuint vertexBuffers
            )
            : vertexArray_(std::move(vertexArray)),
              vertexBuffers_(std::move(vertexBuffers)) {}

    inline const TextureAsset &getTexture() const {
        return *spTexture_;
    }

    inline const TextureAsset setTexture(TextureAsset *spTexture) {
        spTexture_ = std::shared_ptr<TextureAsset>(spTexture);
    }

    inline const GLuint getVAO() const {
        return vertexArray_;
    }

    inline const GLuint getVBOs() const {
        return vertexBuffers_;
    }


private:
    std::shared_ptr<TextureAsset> spTexture_;
    GLuint vertexArray_;
    GLuint vertexBuffers_;
};

#endif //ANDROIDGLINVESTIGATIONS_MODEL_H