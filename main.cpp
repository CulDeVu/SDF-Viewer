#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <stdlib.h>
#include <stdio.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace glm;

static const struct
{
    float x, y;
} vertices[6] =
{
    { -1.0f, -1.0f },
    {  1.0f,  1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },
    {  1.0f,  1.0f },
    {  1.0f, -1.0f },
};

static const char* vertex_shader_text =
"attribute vec2 vPos;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vPos, 0.0, 1.0);\n"
"}\n";

GLFWwindow* window;

float horiz_angle = 0;
float vert_angle = 0;
vec3 origin = vec3(0.);
float plane_height = 0.;
float iTime = 0.;

GLuint vertex_shader, fragment_shader, program;
GLint vpos_location;
GLuint resolution_location,
        origin_location,
        orientation_location,
        plane_location,
        time_location;

string loadFile(char* filename)
{
    std::ifstream t;
    t.open(filename);
    
    string content = "";
    string line = "";
    while (!t.eof())
    {
        std::getline(t, line);
        content.append(line + "\n");
    }

    t.close();
    return content;
}

void reload_shaders()
{
    printf("Reloading shaders...\n");

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    const char* fragShader_text = loadFile("mainShader.fs").c_str();

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragShader_text, NULL);
    glCompileShader(fragment_shader);
    {
        GLint error;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &error);

        //if (error == GL_FALSE)
        {
            GLint logSize = 0;
            glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logSize);

            unsigned char* errorLog = new unsigned char[logSize];
            glGetShaderInfoLog(fragment_shader, logSize, &logSize, (GLchar*)errorLog);
            std::cout << errorLog << std::endl;
        }
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    resolution_location = glGetUniformLocation(program, "resolution");
    origin_location = glGetUniformLocation(program, "origin");
    orientation_location = glGetUniformLocation(program, "orientation");
    plane_location = glGetUniformLocation(program, "plane_height");
    time_location = glGetUniformLocation(program, "iTime");
    printf("%d\n", plane_location);
    vpos_location = glGetAttribLocation(program, "vPos");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 2, (void*) 0);
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void printMat(mat3 mat)
{
    const float *pSource = (const float*)glm::value_ptr(mat);
    for (int i = 0; i < 9; ++i)
    {
        printf("%f ", pSource[i]);
        if (i % 3 == 2)
            printf("\n");
    }
    printf("\n\n");
}

mat3 getRotMat()
{
    mat3 Ry = mat3(vec3(cos(horiz_angle), 0, sin(horiz_angle)),
                    vec3(0, 1, 0),
                    vec3(-sin(horiz_angle), 0, cos(horiz_angle)));
    mat3 Rx = mat3(vec3(1, 0, 0),
                    vec3(0, cos(vert_angle), -sin(vert_angle)),
                    vec3(0, sin(vert_angle), cos(vert_angle)));
    return Ry * Rx;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_K && action == GLFW_PRESS)
        reload_shaders();
}

void doInput()
{
    vec3 dir = vec3(0);

    if (glfwGetKey(window, GLFW_KEY_A))
        dir += vec3(-1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_D))
        dir += vec3(1, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_W))
        dir += vec3(0, 0, -1);
    if (glfwGetKey(window, GLFW_KEY_S))
        dir += vec3(0, 0, 1);
    if (glfwGetKey(window, GLFW_KEY_E))
        dir += vec3(0, 1, 0);
    if (glfwGetKey(window, GLFW_KEY_Q))
        dir += vec3(0, -1, 0);

    if (glfwGetKey(window, GLFW_KEY_UP))
    {
        plane_height += 0.2;
        printf("%f\n", plane_height);
        //printf("safsdfafdsdf");
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN))
        plane_height -= 0.2;

    // Time controller
    if (glfwGetKey(window, GLFW_KEY_RIGHT))
        iTime += 0.4;
    if (glfwGetKey(window, GLFW_KEY_LEFT))
        iTime -= 0.4;

    static bool mouseDown = false;
    static double mouseX = 0, mouseY = 0;

    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (!mouseDown && state == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &mouseX, &mouseY);
        mouseDown = true;
    }
    if (mouseDown  && state != GLFW_PRESS)
        mouseDown = false;

    if (mouseDown)
    {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        horiz_angle += (mx - mouseX) / 400.;
        vert_angle += (my - mouseY) / 400.;

        mouseX = mx;
        mouseY = my;
        //printf("%f %f\n", horiz_angle, vert_angle);
    }

    origin += getRotMat() * dir / 5.f;
    //printf("%f %f %f\n", origin.x, origin.y, origin.z);
}

int main(void)
{
    GLuint vertex_buffer;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    reload_shaders();

    while (!glfwWindowShouldClose(window))
    {
        doInput();

        float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat3 rotMat = getRotMat();
        //printMat(rotMat);

        glUseProgram(program);
        glUniform2f(resolution_location, width, height);
        glUniform3f(origin_location, origin.x, origin.y, origin.z);
        glUniform1f(plane_location, plane_height);
        glUniform1f(time_location, iTime);
        glUniformMatrix3fv(orientation_location, 1, GL_FALSE, glm::value_ptr(rotMat));
        //glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}