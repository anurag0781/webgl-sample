//#include <GL/gl.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <GLES3/gl3.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <pthread.h>
#include <emscripten/html5_webgpu.h>
#include <string>
#include <iostream>
#include <vector>

pthread_t renderingThreadId = 0, render_th_id = 0;

GLuint program;
GLuint vao;
GLuint vbo;


GLuint getBlitProgram()
{
        // Shader source code for a simple 2D texture shader
    const char* vertexShaderSource =
    "#version 300 es\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = vec4(aPos.xyz, 1.0);  \n"
    "   TexCoord = aTexCoord;\n"
    "}                            \n";
    const char* fragmentShaderSource =
    "#version 300 es\n"
    "precision mediump float;\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D textureSampler;\n"
    "void main()                                  \n"
    "{                                            \n"
    "  // FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "  FragColor = texture(textureSampler, TexCoord);\n"
    "}                                            \n";
    // Compile shaders and link program
    GLuint vertexShader, fragmentShader, shaderProgram;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

std::vector<GLuint> createVertexBuffers()
{
    float vertices[] = {
        // Positions       // Texture Coordinates
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };
    GLuint indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return {VAO, VBO, EBO};
}

void drawTextureOnScreen(GLuint srcTexture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind back to default framebuffer
    glBindTexture(GL_TEXTURE_2D, srcTexture);
    // Set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set the texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLuint blitProgram = getBlitProgram();
    //Set up vertex data and buffers
    auto vertexBuffers = createVertexBuffers();
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);
    glUniform1i(glGetUniformLocation(blitProgram, "textureSampler"), 0);
    // Use shader program
    glUseProgram(blitProgram);
    // Draw the quad
    glBindVertexArray(vertexBuffers[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


GLuint drawLine(bool shouldDrawOnTexture)
{
    // Setup to drawline with width
    const float angle = M_PI_4;  // Angle in degrees
    const float length = 0.8f;  // Length of the line
    const float thickness = 0.1f;  // Thickness of the line

    // Calculate the half-width and half-height of the rectangle
    float halfWidth = thickness / 2.0f;
    float halfHeight = length / 2.0f;

    // Calculate the rotation matrix for the specified angle
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    // Calculate the vertices of the rectangle
    std::vector<GLfloat> lineVertices = {
        -halfWidth, -halfHeight,  // Bottom-left
        halfWidth, -halfHeight,   // Bottom-right
        -halfWidth, halfHeight,    // Top-left
        halfWidth, halfHeight      // Top-right
    };

    // Rotate the rectangle vertices
    for (int i = 0; i < lineVertices.size(); i += 2) {
        float x = lineVertices[i];
        float y = lineVertices[i + 1];

        lineVertices[i] = cosAngle * x - sinAngle * y;
        lineVertices[i + 1] = sinAngle * x + cosAngle * y;
    }

    // Declare buffer Data
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(GLfloat), lineVertices.data(), GL_STATIC_DRAW);

    // Specify vertex attribute pointers
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);


    // Create texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 600, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (!shouldDrawOnTexture)
        glBindTexture(GL_TEXTURE_2D, 0);

    // Framebuffer setup
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

    if (!shouldDrawOnTexture)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Draw to texture
    glUseProgram(program);
    glBindVertexArray(vao);
    // For line
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

GLuint drawCircle(bool shouldDrawOnTexture)
{
    // Setup vertex data according without texture coordinates
    const int segments = 50;
    const float radius = 0.5;
    const float lineWidth = 0.1;
    std::vector<GLfloat> vertices;

    for (int i = 0; i <= segments; ++i)
    {
        float angle = (i / static_cast<float>(segments)) * 2.0 * 3.14159;
        float x_outer = cos(angle) * (radius + lineWidth / 2.0);
        float y_outer = sin(angle) * (radius + lineWidth / 2.0);
        float x_inner = cos(angle) * (radius - lineWidth / 2.0);
        float y_inner = sin(angle) * (radius - lineWidth / 2.0);

        vertices.push_back(x_outer);
        vertices.push_back(y_outer);

        vertices.push_back(x_inner);
        vertices.push_back(y_inner);
    }

    // Duplicate the first two vertices to close the circle
    vertices.push_back(vertices[0]);
    vertices.push_back(vertices[1]);

    vertices.push_back(vertices[2]);
    vertices.push_back(vertices[3]);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STREAM_DRAW);

    // Specify vertex attribute pointers
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Create texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 600, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (!shouldDrawOnTexture)
        glBindTexture(GL_TEXTURE_2D, 0);

    // Framebuffer setup
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

    if (!shouldDrawOnTexture)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Draw to texture
    glUseProgram(program);
    glBindVertexArray(vao);
    // For circle
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size() / 2);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

GLuint drawCircleOnTextureWithUV()
{
    // Set up vertex data
    const int segments = 50;
    const float radius = 0.5;
    const float lineWidth = 0.1;
    std::vector<GLfloat> vertices;

    for (int i = 0; i <= segments; ++i)
    {
        float angle = (i / static_cast<float>(segments)) * 2.0 * 3.14159;
        float x_outer = cos(angle) * (radius + lineWidth / 2.0);
        float y_outer = sin(angle) * (radius + lineWidth / 2.0);
        float x_inner = cos(angle) * (radius - lineWidth / 2.0);
        float y_inner = sin(angle) * (radius - lineWidth / 2.0);

        float u = (x_outer + 1.0) / 2.0;
        float v = (y_outer + 1.0) / 2.0;

        vertices.push_back(x_outer);
        vertices.push_back(y_outer);
        vertices.push_back(u);
        vertices.push_back(v);

        u = (x_inner + 1.0) / 2.0;
        v = (y_inner + 1.0) / 2.0;

        vertices.push_back(x_inner);
        vertices.push_back(y_inner);
        vertices.push_back(u);
        vertices.push_back(v);
    }

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    // Specify vertex attribute pointers
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 600, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // glBindTexture(GL_TEXTURE_2D, 0);

    // Framebuffer setup
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Draw to texture
    glUseProgram(program);
    glBindVertexArray(vao);
    // For circle
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size() / 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

void init() {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE _context;
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.enableExtensionsByDefault = 1;
    // attrs.explicitSwapControl = true;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attrs.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
    // attrs.renderViaOffscreenBackBuffer = true;
    attrs.depth = 1;
    attrs.stencil = 1;
    attrs.antialias = 1;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    _context = emscripten_webgl_create_context("#myCanvas", &attrs);
    if (_context > 0)
    {
        printf("\n\nCreation of context succeeded!!!\n\n");
    }
    else
    {
         printf("\n\nCreation of context failed!!!\n\n");
    }

    emscripten_webgl_make_context_current(_context);

    // Clear the screen to yellow
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);

    // Create shaders
    // const char* vertexShaderSource = "#version 300 es\n"
    //                                  "precision highp float;\n"
    //                                  "layout (location = 0) in vec2 position;\n"
    //                                  "void main() {\n"
    //                                  "    gl_Position = vec4(position, 0.0, 1.0);\n"
    //                                  "}";

    // const char *vertexShaderSource =
    //                                 "#version 300 es\n"
    //                                 "layout (location = 0) in vec2 xy;\n"
    //                                 "layout (location = 1) in vec2 uv;\n"
    //                                 "out vec2 v_texturePos;\n"
    //                                 "void main() {\n"
    //                                 "    gl_Position = vec4(xy, 0.0, 1.0);\n"
    //                                 "    v_texturePos = uv;\n"
    //                                 "}";

    const char *vertexShaderSource =
                                    "#version 300 es\n"
                                    "layout (location = 0) in vec2 xy;\n"
                                    "void main() {\n"
                                    "    gl_Position = vec4(xy, 0.0, 1.0);\n"
                                    "}";

    const char* fragmentShaderSource = "#version 300 es\n"
                                       "precision highp float;\n"
                                       "out vec4 FragColor;\n"
                                       "void main() {\n"
                                       "    FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                                       "}";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create shader program
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Create VAO and VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    bool shouldDrawOnTexture = true;
    GLuint textureId = drawCircle(shouldDrawOnTexture);
    // GLuint textureId = drawLine(shouldDrawOnTexture);

    if (shouldDrawOnTexture)
        drawTextureOnScreen(textureId);
}

void render() {
    // glClear(GL_COLOR_BUFFER_BIT);

    // glUseProgram(program);
    // glBindVertexArray(vao);

    // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // glBindVertexArray(0);
    // glUseProgram(0);
}

void processPendingJobs()
{
    // EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_get_current_context();
    // // printf("\n\nInside worker thread.\n\n");
    // EM_ASM({
    //     // var gl = Emval.toValue($0);
    //     // console.log(gl);
    //     var gl = Module.GL.currentContext.GLctx;
    //     // var gl = Module.GL.getContext($0).GLctx;
    //     // var canvas = Module.findCanvasEventTarget('#webgl_canvas');
    //     // var gl = canvas.getContext('webgl');
    //     gl.clearColor(0.0, 0.0, 1.0, 1.0);
    //     gl.clear(gl.COLOR_BUFFER_BIT);

    //     // var webgpuContext = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.getContext('webgl');
    //     // webgpuContext.clearColor(0.0, 1.0, 0.0, 1.0);
    //     // webgpuContext.clear(gl.COLOR_BUFFER_BIT);

    //     var webgpuContext = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.getContext('2d');
    //     var width = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.width;
    //     var height = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.height;
    //     webgpuContext.fillStyle = 'red';
    //     webgpuContext.fillRect(0, 0, width, height);

        
        
    // }, ctx);

    

}

void *renderingThreadEntry(void *arg)
{
    // render_th_id = pthread_self();
    // printf("\n\nInside renderingThreadEntry().\n\n");
    // EM_ASM({
    //     // console.log("Inside processPendingJobs().1");
    //     // const offscreen = new OffscreenCanvas(256, 256);
    //     // offscreen.id = "webgpu_canvas";
    //     // var webgpuContext = offscreen.getContext('webgpu');
    //     // console.log(webgpuContext);
    // });
    // // webgpu calls which uses webgpu_canvas

    // std::string jsCanvasSelector = "#webgl_canvas";
    // EmscriptenWebGLContextAttributes attrs;
    // emscripten_webgl_init_context_attributes(&attrs);
    // attrs.enableExtensionsByDefault = 1;
    // attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    // attrs.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
    // attrs.depth = 1;
    // attrs.stencil = 1;
    // attrs.antialias = 1;
    // attrs.majorVersion = 2;
    // attrs.minorVersion = 0;
    // attrs.premultipliedAlpha = true;
    
    // EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(jsCanvasSelector.c_str(), &attrs);
    // if (ctx > 0)
    // {
    //     printf("\n\nCreation of context succeeded in the worker!!!\n\n");
    // }
    // else
    // {
    //      printf("\n\nCreation of context failed in the worker!!!\n\n");
    // }
    // emscripten_webgl_make_context_current(ctx);

    // emscripten_set_main_loop(processPendingJobs, 60, 1);
    init();
    return nullptr;
}

int main() 
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    emscripten_pthread_attr_settransferredcanvases(&attr, "#myCanvas");
    pthread_create(&renderingThreadId, &attr, renderingThreadEntry, (void *)nullptr);

    // init();
    // emscripten_set_main_loop(render, 0, 1);
    // render();

    return 0;
}
