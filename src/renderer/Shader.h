#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void bind() const;
    void unbind() const;

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat4(const std::string& name, const glm::mat4& value);
    void setMat4Array(const std::string& name, const glm::mat4* values, int count);

private:
    GLuint m_program = 0;
    std::unordered_map<std::string, GLint> m_uniformCache;

    GLint getUniformLocation(const std::string& name);
    static std::string readFile(const std::string& path);
    static GLuint compileShader(GLenum type, const std::string& source);
};
