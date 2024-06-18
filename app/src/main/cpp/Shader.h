#ifndef ANDROIDGLINVESTIGATIONS_SHADER_H
#define ANDROIDGLINVESTIGATIONS_SHADER_H

#include <string>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

class Cube;

/*!
 * A class representing a simple shader program. It consists of vertex and fragment components. The
 * input attributes are a position (as a Vector3) and a uv (as a Vector2). It also takes a uniform
 * to be used as the entire model/view/projection matrix. The shader expects a single texture for
 * fragment shading, and does no other lighting calculations (thus no uniforms for lights or normal
 * attributes).
 */
class Shader {
public:
    Shader(AAssetManager *assetManager, const std::string &vertexPath,const std::string &fragmentPath);

    ~Shader() {
        if (program_) {
            glDeleteProgram(program_);
            program_ = 0;
        }
    }

    void activate() const;

    void deactivate() const;

private:
    static GLuint loadShader(GLenum shaderType, const std::string &shaderSource);

    std::string loadFile(AAssetManager *assetManager, const std::string fileName);
    GLuint program_;
};

#endif //ANDROIDGLINVESTIGATIONS_SHADER_H