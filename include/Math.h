#pragma once
#include <cmath>

struct Vec2 {
  float x;
  float y;

  Vec2() : x(0), y(0) {}
  Vec2(float x, float y) : x(x), y(y) {}
  Vec2(const Vec2 &) = default;
  ~Vec2() = default;

  Vec2 operator-() const { return Vec2{-x, -y}; }
  Vec2 operator+(const Vec2 &other) const {
    return Vec2{x + other.x, y + other.y};
  }
  Vec2 operator-(const Vec2 &other) const {
    return Vec2{x - other.x, y - other.y};
  }
  Vec2 operator*(float scalar) const { return Vec2{x * scalar, y * scalar}; }
  Vec2 operator/(float scalar) const { return Vec2{x / scalar, y / scalar}; }
  Vec2 operator+=(const Vec2 &other) {
    x += other.x;
    y += other.y;
    return *this;
  }
  Vec2 operator-=(const Vec2 &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
  Vec2 operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }
  Vec2 operator/=(float scalar) {
    x /= scalar;
    y /= scalar;
    return *this;
  }
  float lengthSq() const { return x * x + y * y; }
  float length() const { return std::sqrt(lengthSq()); }
};
