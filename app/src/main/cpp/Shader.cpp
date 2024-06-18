#include "Shader.h"

#include "AndroidOut.h"

Shader::Shader(AAssetManager *assetManager, const std::string &vertexPath,const std::string &fragmentPath) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, loadFile(assetManager,vertexPath));
    if (!vertexShader) {
        return;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, loadFile(assetManager,fragmentPath));
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            glDeleteProgram(program);
        } else {
            program_ = program;
        }
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

GLuint Shader::loadShader(GLenum shaderType, const std::string &shaderSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        auto *shaderRawString = (GLchar *) shaderSource.c_str();
        GLint shaderLength = shaderSource.length();
        glShaderSource(shader, 1, &shaderRawString, &shaderLength);
        glCompileShader(shader);

        GLint shaderCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);

        // If the shader doesn't compile, log the result to the terminal for debugging
        if (!shaderCompiled) {
            GLint infoLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

            if (infoLength) {
                auto *infoLog = new GLchar[infoLength];
                glGetShaderInfoLog(shader, infoLength, nullptr, infoLog);
                aout << "Failed to compile with:\n" << infoLog << std::endl;
                delete[] infoLog;
            }

            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

std::string Shader::loadFile(AAssetManager *assetManager, const std::string fileName) {
    auto shaderAsset = AAssetManager_open(
            assetManager,
            fileName.c_str(),
            AASSET_MODE_BUFFER);
    off_t file_size = AAsset_getLength(shaderAsset);
    std::string file_buffer;
    file_buffer.resize(file_size);
    AAsset_read(shaderAsset, &file_buffer.front(), file_size);
    AAsset_close(shaderAsset);

    return file_buffer;
}

void Shader::activate() const {
    glUseProgram(program_);
}

void Shader::deactivate() const {
    glUseProgram(0);
}

GLuint Shader::getProgram() const {
    return program_;
}