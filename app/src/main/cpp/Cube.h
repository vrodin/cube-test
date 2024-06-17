#ifndef ANDROIDGLINVESTIGATIONS_MODEL_H
#define ANDROIDGLINVESTIGATIONS_MODEL_H

#include <vector>
#include "TextureAsset.h"
#include <glm/glm.hpp>

class Cube {
public:
    inline Cube(
            std::shared_ptr<TextureAsset> spTexture,
            GLuint vertexArray,
            std::vector<GLuint> vertexBuffers
            )
            : spTexture_(std::move(spTexture)),
              vertexArray_(std::move(vertexArray)),
              vertexBuffers_(std::move(vertexBuffers)) {}

    inline const TextureAsset &getTexture() const {
        return *spTexture_;
    }

    inline const GLuint getVAO() const {
        return vertexArray_;
    }

    inline const GLuint* getVBOs() const {
        return vertexBuffers_.data();
    }


private:
    std::shared_ptr<TextureAsset> spTexture_;
    GLuint vertexArray_;
    std::vector<GLuint> vertexBuffers_;
};

#endif //ANDROIDGLINVESTIGATIONS_MODEL_H