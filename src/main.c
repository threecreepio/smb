#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "glad/gl.h"
#include <GLFW/glfw3.h>
#include "game.c"

#define errlog stderr

struct VTexData { GLfloat pos[2]; GLfloat uv[2]; };
static const struct VTexData vertices[] =
{
  { { -1.f, -1.f }, { 0.f, 1.f } },
  { { -1.f,  1.f }, { 0.f, 0.f } },
  { {  1.f,  1.f }, { 1.f, 0.f } },

  { { -1.f, -1.f }, { 0.f, 1.f } },
  { {  1.f,  1.f }, { 1.f, 0.f } },
  { {  1.f, -1.f }, { 1.f, 1.f } },
};

static const char* vertex_shader_text =
"#version 330\n"
"in vec2 vPos;\n"
"in vec2 vUV;\n"
"out vec2 uv;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vPos, 0.0, 1.0);\n"
"    uv = vUV;\n"
"}\n";

extern const char fragment_shader_code[];
extern const uint32_t fragment_shader_size;

static void GlfwErrorCallback(int error, const char* description)
{
    fprintf(errlog, "glfw error %s\n", description);
}

static void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}



GLuint LoadAreaTexture(int areanumber) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 0x10, 0x200, 0, GL_RED, GL_UNSIGNED_BYTE, areadata[areanumber]);
    return textureID;
}

int LoadShader(const GLuint shader, const char *source) {
    GLint ok = 0;
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        GLchar *errorLog = calloc(maxLength, 1);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        glDeleteShader(shader);
        printf("shader load error: %s\n", errorLog);
        free(errorLog);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit()) {
        const char* description;
        glfwGetError(&description);
        fprintf(errlog, "glfw init error %s\n", description);
        return -1;
    }

    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1, 1, "Super Mario C", NULL, NULL);
    if (!window) {
        glfwTerminate();
        fprintf(errlog, "failed to create window\n");
        return -1;
    }

    glfwSetKeyCallback(window, GlfwKeyCallback);
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(errlog, "failed to init gl context\n");
        return -1;
    }

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (0 != LoadShader(vertex_shader, vertex_shader_text)) {
        return -1;
    }

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    if (0 != LoadShader(fragment_shader, fragment_shader_code)) {
        return -1;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    const GLint vuv_location = glGetAttribLocation(program, "vUV");
    const GLint scrollx_location = glGetUniformLocation(program, "scrollx");

    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(struct VTexData), (void*) offsetof(struct VTexData, pos));
    glEnableVertexAttribArray(vuv_location);
    glVertexAttribPointer(vuv_location, 2, GL_FLOAT, GL_FALSE, sizeof(struct VTexData), (void*) offsetof(struct VTexData, uv));

    int scale = 3;
    glViewport(0, 0, 256 * scale, 240 * scale);
    glfwSetWindowSize(window, 256 * scale, 240 * scale);

    GLuint tex0 = LoadAreaTexture(0);
    GLuint tex1 = LoadAreaTexture(1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex1);

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "tex0"), 0);
    int scrollx = 0;
    int scrolly = 0;
    int frames = 0;
    double ts = glfwGetTime();
    double tsnow = glfwGetTime();

    struct gamestate game = { 0 };
    while (!glfwWindowShouldClose(window)) {
        frames += 1;
        //double now = glfwGetTime();
        //if (now - ts >= 10.0) {
        //    printf("FPS: %f\n", (double)frames / (now - ts));
        //    ts = now;
        //    frames = 0;
        //}
        nmi(&game);

        glUniform1i(scrollx_location, game.scrollx);
        //if (frames % 0x3 == 0) { scrollx += 1; }
        scrollx += 1;
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glFinish();
        glfwSwapBuffers(window);
        glfwPollEvents();

        while (1) {
            double pending = glfwGetTime();
            if (pending - tsnow >= 1.0 / 60.098814) {
                tsnow = pending;
                break;
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
