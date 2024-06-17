#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <glm/glm.hpp>
#include <vector>
#include <android/imagedecoder.h>

#include "AndroidOut.h"
#include "Shader.h"
#include "TextureAsset.h"

// Vertex shader, you'd typically load this from assets
static const char *vertex = R"vertex(#version 300 es
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 inUV;

out vec2 fragUV;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(position, 1.0f);
    fragUV = vec2(inUV.x, inUV.y);
}
)vertex";

// Fragment shader, you'd typically load this from assets
static const char *fragment = R"fragment(#version 300 es
precision mediump float;
in vec2 fragUV;

out vec4 color;

uniform sampler2D texture1;

void main()
{
    color = texture(texture1, fragUV);
}
)fragment";

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::render() {
    updateRenderArea();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!figures_.empty()) {
        shader_->activate();
        for (const auto &figure: figures_) {
            glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.f), (float)width_ / (float)height_, 0.1f, 100.0f);
            glm::mat4 view = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3( 0.0f,  0.0f,  0.0f));
            model = glm::rotate(model, movementSpeed_ * 10.0f, glm::vec3(0.0f, 10.0f, 0.0f));

            glm::mat4 result = projectionMatrix * view * model;
            shader_->drawFigure(figure, result);
        }
    }

    // Present the rendered image. This is an implicit glFlush.
    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
    // Choose your render attributes
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    // The default display is probably what you want on Android
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    // figure out how many configs there are
    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    // get the list of configurations
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    // Find a config we like.
    // Could likely just grab the first if we don't care about anything else in the config.
    // Otherwise hook in your own heuristic
    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    // create the proper window surface
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    // get some window metrics
    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    // make width and height invalid so it gets updated the first frame in @a updateRenderArea()
    width_ = -1;
    height_ = -1;

    shader_ = std::unique_ptr<Shader>(
            Shader::loadShader(vertex, fragment, "position", "inUV", "projection"));
    assert(shader_);

    // setup any other gl related global states
    glClearColor(1.0f,1.0f,1.0f,1.0f);

    // enable alpha globally for now, you probably don't want to do this in a game
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // get some demo models into memory
    createModels();
}

void Renderer::updateRenderArea() {
    EGLint width;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

    EGLint height;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);

        // make sure that we lazily recreate the projection matrix before we render
        shaderNeedsNewProjectionMatrix_ = true;
    }
}

void Renderer::createModels() {

    static const float s = 1.0f;
    static const float cubePositions[36][3] = {
            {-s, s, s}, { s, s, s}, { s,-s, s}, {-s,-s, s}, // front
            { s, s,-s}, {-s, s,-s}, {-s,-s,-s}, { s,-s,-s}, // back
            {-s, s,-s}, { s, s,-s}, { s, s, s}, {-s, s, s}, // top
            { s,-s,-s}, {-s,-s,-s}, {-s,-s, s}, { s,-s, s}, // bottom
            {-s, s,-s}, {-s, s, s}, {-s,-s, s}, {-s,-s,-s}, // left
            { s, s, s}, { s, s,-s}, { s,-s,-s}, { s,-s, s}  // right
    };

    static const float cubeTexcoords[24][2] = {
            {0.0f,1.0f}, {1.0f,1.0f}, {1.0f,0.0f}, {0.0f,0.0f}, // front
            {0.0f,1.0f}, {1.0f,1.0f}, {1.0f,0.0f}, {0.0f,0.0f}, // back
            {0.0f,1.0f}, {1.0f,1.0f}, {1.0f,0.0f}, {0.0f,0.0f}, // top
            {0.0f,1.0f}, {1.0f,1.0f}, {1.0f,0.0f}, {0.0f,0.0f}, // bottom
            {0.0f,1.0f}, {1.0f,1.0f}, {1.0f,0.0f}, {0.0f,0.0f}, // left
            {0.0f,1.0f}, {1.0f,1.0f}, {1.0f,0.0f}, {0.0f,0.0f}  // right
    };

    static const uint16_t cubeIndices[36] = {
            0, 3, 1,  1, 3, 2, // front
            4, 7, 5,  5, 7, 6, // back
            8,11, 9,  9,11,10, // top
            12,15,13, 13,15,14, // bottom
            16,19,17, 17,19,18, // left
            20,23,21, 21,23,22  // right
    };
    auto assetManager = app_->activity->assetManager;
    auto spAndroidRobotTexture = TextureAsset::loadAsset(assetManager, "b2s_upscaled.png");

    GLuint m_vertexArray;
    GLuint m_vertexBuffer[3];
    glGenVertexArrays(1, &m_vertexArray);
    glBindVertexArray(m_vertexArray);

    glGenBuffers(3, m_vertexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, 24 * (3 * sizeof(float)), cubePositions, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, 24 * (2 * sizeof(float)),cubeTexcoords, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vertexBuffer[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36,cubeIndices, GL_DYNAMIC_DRAW);

    figures_.emplace_back(spAndroidRobotTexture, m_vertexArray, std::vector<GLuint> (m_vertexBuffer, m_vertexBuffer + 3));

}

void Renderer::handleInput() {
    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        // no inputs yet.
        return;
    }

    // handle motion events (motionEventsCounts can be 0).
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // Find the pointer index, mask and bitshift to turn it into a readable value.
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        aout << "Pointer(s): ";

        // get the x and y position of this event if it is not ACTION_MOVE.
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        long startTime;
        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                movementSpeed_ = 0;
                startTime = std::time(nullptr);
                break;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Down";
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    movementSpeed_ = (pointer.rawY - y) / (std::time(nullptr) - startTime);
                    if (index != (motionEvent.pointerCount - 1)) aout << ",";
                    aout << " ";
                }
                aout << "Pointer Move";
                break;
            default:
                aout << "Unknown MotionEvent Action: " << action;
        }
        aout << std::endl;
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

}