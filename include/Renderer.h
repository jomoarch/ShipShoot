#pragma once
#include <glad/glad.h>
#include <vector>

class Renderer {
public:
  Renderer();
  ~Renderer();
  bool init();

  // primitive drawing operations
  void clear();
  void setProjection(float width, float height);
  void drawPixel(float x, float y, float r, float g, float b);
  void drawLine(float x1, float y1, float x2, float y2, float r, float g,
                float b);
  void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                    float r, float g, float b);
  void drawCircle(float cx, float cy, float radius, float r, float g, float b,
                  bool hollow = false);

private:
  GLuint vao, vbo;
  GLuint shaderProgram;
  GLuint lineShaderProgram;
  GLuint triShaderProgram;
  float projMatrix[16];

  void setupShaders();
  void setupBuffers();
};
