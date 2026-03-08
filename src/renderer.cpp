#include "renderer.h"
#include <iostream>
#include <vector>
#include <cmath>

// basic passthrough vertex shader
static const char *basicVertex = R"(
#version 330 core
layout(location=0) in vec2 aPos;
uniform mat4 projection;
void main(){
    gl_Position = projection * vec4(aPos,0.0,1.0);
}
)";

// basic fragment shader with uniform color
static const char *basicFragment = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main(){
    FragColor = vec4(uColor,1.0);
}
)";

Renderer::Renderer()
    : vao(0), vbo(0), shaderProgram(0), lineShaderProgram(0),
      triShaderProgram(0) {
  for (int i = 0; i < 16; i++)
    projMatrix[i] = (i % 5 == 0 ? 1.0f : 0.0f);
}

Renderer::~Renderer() {
  if (vao)
    glDeleteVertexArrays(1, &vao);
  if (vbo)
    glDeleteBuffers(1, &vbo);
  if (shaderProgram)
    glDeleteProgram(shaderProgram);
}

bool Renderer::init() {
  setupShaders();
  setupBuffers();
  return true;
}

void Renderer::setupShaders() {
  auto compile = [&](const char *src, GLenum type) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
      char buf[512];
      glGetShaderInfoLog(s, 512, nullptr, buf);
      std::cerr << "Shader compile error:" << buf << std::endl;
    }
    return s;
  };
  GLuint vert = compile(basicVertex, GL_VERTEX_SHADER);
  GLuint frag = compile(basicFragment, GL_FRAGMENT_SHADER);
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vert);
  glAttachShader(shaderProgram, frag);
  glLinkProgram(shaderProgram);
  int ok;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
  if (!ok) {
    char buf[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, buf);
    std::cerr << "Program link error:" << buf << std::endl;
  }
  glDeleteShader(vert);
  glDeleteShader(frag);
  lineShaderProgram = shaderProgram;
  triShaderProgram = shaderProgram;
}

void Renderer::setupBuffers() {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Renderer::clear() { glClear(GL_COLOR_BUFFER_BIT); }

void Renderer::setProjection(float width, float height) {
  for (int i = 0; i < 16; i++)
    projMatrix[i] = 0;
  projMatrix[0] = 2.0f / width;
  projMatrix[5] = 2.0f / height;
  projMatrix[10] = 1.0f;
  projMatrix[12] = -1.0f;
  projMatrix[13] = -1.0f;
  projMatrix[15] = 1.0f;
}

void Renderer::drawPixel(float x, float y, float r, float g, float b) {
  float verts[2] = {x, y};
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
  glUseProgram(shaderProgram);
  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);
  GLint col = glGetUniformLocation(shaderProgram, "uColor");
  glUniform3f(col, r, g, b);
  glDrawArrays(GL_POINTS, 0, 1);
}

void Renderer::drawLine(float x1, float y1, float x2, float y2, float r,
                        float g, float b) {
  float verts[4] = {x1, y1, x2, y2};
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
  glUseProgram(shaderProgram);
  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);
  GLint col = glGetUniformLocation(shaderProgram, "uColor");
  glUniform3f(col, r, g, b);
  glDrawArrays(GL_LINES, 0, 2);
}

void Renderer::drawTriangle(float x1, float y1, float x2, float y2, float x3,
                            float y3, float r, float g, float b) {
  float verts[6] = {x1, y1, x2, y2, x3, y3};
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
  glUseProgram(shaderProgram);
  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);
  GLint col = glGetUniformLocation(shaderProgram, "uColor");
  glUniform3f(col, r, g, b);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::drawCircle(float cx, float cy, float radius, float r, float g,
                          float b, bool hollow) {
  const int seg = 16;
  std::vector<float> verts;
  verts.reserve(seg * 2);
  for (int i = 0; i < seg; i++) {
    float ang = (2.0f * M_PI * i) / seg;
    verts.push_back(cx + cos(ang) * radius);
    verts.push_back(cy + sin(ang) * radius);
  }
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
               GL_DYNAMIC_DRAW);
  glUseProgram(shaderProgram);
  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);
  GLint col = glGetUniformLocation(shaderProgram, "uColor");
  glUniform3f(col, r, g, b);
  if (hollow)
    glDrawArrays(GL_LINE_LOOP, 0, seg);
  else
    glDrawArrays(GL_TRIANGLE_FAN, 0, seg);
}
