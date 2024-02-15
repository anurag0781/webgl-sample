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

pthread_t renderingThreadId = 0, render_th_id = 0;

GLuint program;
GLuint vao;
GLuint vbo;

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

    // Clear the screen to green
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);


    // Create shaders
    const char* vertexShaderSource = "#version 300 es\n"
                                     "precision mediump float;\n"
                                     "layout (location = 0) in vec2 position;\n"
                                     "void main() {\n"
                                     "    gl_Position = vec4(position, 0.0, 1.0);\n"
                                     "}";

    const char* fragmentShaderSource = "#version 300 es\n"
                                       "precision mediump float;\n"
                                       "out vec4 FragColor;\n"
                                       "void main() {\n"
                                       "    FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
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
    GLfloat vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         0.0f,  1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);
}

void processPendingJobs()
{
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_get_current_context();
    // printf("\n\nInside worker thread.\n\n");
    EM_ASM({
        // var gl = Emval.toValue($0);
        // console.log(gl);
        var gl = Module.GL.currentContext.GLctx;
        // var gl = Module.GL.getContext($0).GLctx;
        // var canvas = Module.findCanvasEventTarget('#webgl_canvas');
        // var gl = canvas.getContext('webgl');
        gl.clearColor(0.0, 0.0, 1.0, 1.0);
        gl.clear(gl.COLOR_BUFFER_BIT);

        // var webgpuContext = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.getContext('webgl');
        // webgpuContext.clearColor(0.0, 1.0, 0.0, 1.0);
        // webgpuContext.clear(gl.COLOR_BUFFER_BIT);

        var webgpuContext = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.getContext('2d');
        var width = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.width;
        var height = Module.findCanvasEventTarget('#webgpu_canvas').offscreenCanvas.height;
        webgpuContext.fillStyle = 'red';
        webgpuContext.fillRect(0, 0, width, height);

        
        
    }, ctx);

    

}

void *renderingThreadEntry(void *arg)
{
    render_th_id = pthread_self();
    printf("\n\nInside renderingThreadEntry().\n\n");
    EM_ASM({
        // console.log("Inside processPendingJobs().1");
        // const offscreen = new OffscreenCanvas(256, 256);
        // offscreen.id = "webgpu_canvas";
        // var webgpuContext = offscreen.getContext('webgpu');
        // console.log(webgpuContext);
    });
    // webgpu calls which uses webgpu_canvas

    std::string jsCanvasSelector = "#webgl_canvas";
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.enableExtensionsByDefault = 1;
    attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attrs.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
    attrs.depth = 1;
    attrs.stencil = 1;
    attrs.antialias = 1;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.premultipliedAlpha = true;
    
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(jsCanvasSelector.c_str(), &attrs);
    if (ctx > 0)
    {
        printf("\n\nCreation of context succeeded in the worker!!!\n\n");
    }
    else
    {
         printf("\n\nCreation of context failed in the worker!!!\n\n");
    }
    emscripten_webgl_make_context_current(ctx);

    emscripten_set_main_loop(processPendingJobs, 60, 1);
    return nullptr;
}

int main() 
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    emscripten_pthread_attr_settransferredcanvases(&attr, "#webgl_canvas,#webgpu_canvas");
    pthread_create(&renderingThreadId, &attr, renderingThreadEntry, (void *)nullptr);

    init();
    emscripten_set_main_loop(render, 0, 1);

    return 0;
}
