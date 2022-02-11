/**
* @file shader.h
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#pragma once
#include <iostream>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace cs460 {
enum shader_type
{
  VERTEX,
  FRAGMENT,
  GEOMETRY,
  TESS_CONTROL,
  TESS_EVALUATION,
  COMPUTE
};

class shader
{
public:
static shader * CreateShaderProgram(const std::string & vertShader,
                                         const std::string & fragShader);
shader();
~shader();
bool CompileShaderFromFile(const char * fileName, shader_type type);
bool CompileShaderFromString(const std::string & source, shader_type type);
bool Link();
bool Validate();
void Use() const;
std::string Log() const;
int  GetHandle() const;
bool IsLinked() const;
void BindAttribLocation(unsigned int location, const char * name);
void BindFragDataLocation(unsigned int location, const char * name);
void SetUniform(const std::string & name, float x, float y, float z) const;
void SetUniform(const std::string & name, const glm::vec2 & v) const;
void SetUniform(const std::string & name, const glm::vec3 & v) const;
void SetUniform(const std::string & name, const glm::vec4 & v) const;
void SetUniform(const std::string & name, const glm::mat4 & m) const;
void SetUniform(const std::string & name, const glm::mat3 & m) const;
void SetUniform(const std::string & name, float val) const;
void SetUniform(const std::string & name, int val) const;
void SetUniform(const std::string & name, bool val) const;
void SetSubroutineUniform(const std::string & name, const std::string & funcName) const;
void PrintActiveUniforms() const;
void PrintActiveAttribs() const;

private:
int  GetUniformLocation(const std::string & name) const;

int         handle_;
bool        linked_;
std::string log_string_;
};
}