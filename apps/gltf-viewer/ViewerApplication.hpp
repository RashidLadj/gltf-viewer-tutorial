#pragma once

#include "tiny_gltf.h"
#include "utils/GLFWHandle.hpp"
#include "utils/cameraControllerInterface.hpp"
#include "utils/filesystem.hpp"
#include "utils/shaders.hpp"

class ViewerApplication
{
public:
  ViewerApplication(const fs::path &appPath, uint32_t width, uint32_t height,
      const fs::path &gltfFile, const std::vector<float> &lookatArgs,
      const std::string &vertexShader, const std::string &fragmentShader,
      const fs::path &output);

  int run();

private:
  // A range of indices in a vector containing Vertex Array Objects
  struct VaoRange
  {
    GLsizei begin; // Index of first element in vertexArrayObjects
    GLsizei count; // Number of elements in range
  };

  struct Light
  {
    // vec3 position; // No longer necessery when using directional lights.
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
  };

  GLsizei m_nWindowWidth = 1280;
  GLsizei m_nWindowHeight = 720;

  const fs::path m_AppPath;
  const std::string m_AppName;
  const fs::path m_ShadersRootPath;

  fs::path m_gltfFilePath;
  std::string m_vertexShader = "forward.vs.glsl";
  std::string m_fragmentShader = "pbr_directional_light.fs.glsl";

  bool m_hasUserCamera = false;
  Camera m_userCamera;

  fs::path m_OutputPath;

  // Order is important here, see comment below
  const std::string m_ImGuiIniFilename;
  // Last to be initialized, first to be destroyed:
  GLFWHandle m_GLFWHandle{int(m_nWindowWidth), int(m_nWindowHeight),
      "glTF Viewer",
      m_OutputPath.empty()}; // show the window only if m_OutputPath is empty
  /*
    ! THE ORDER OF DECLARATION OF MEMBER VARIABLES IS IMPORTANT !
    - m_ImGuiIniFilename.c_str() will be used by ImGUI in ImGui::Shutdown, which
    will be called in destructor of m_GLFWHandle. So we must declare
    m_ImGuiIniFilename before m_GLFWHandle so that m_ImGuiIniFilename
    destructor is called after.
    - m_GLFWHandle must be declared before the creation of any object managing
    OpenGL resources (e.g. GLProgram, GLShader) because it is responsible for
    the creation of a GLFW windows and thus a GL context which must exists
    before most of OpenGL function calls.
  */
  bool loadGltfFile(tinygltf::Model &model);
  std::vector<GLuint> createBufferObjects(
      const tinygltf::Model &model, bool normalMapping);
  std::vector<GLuint> createVertexArrayObjects(const tinygltf::Model &model,
      const std::vector<GLuint> &bufferObjects,
      std::vector<VaoRange> &meshIndexToVaoRange, bool normalMapping);
  std::vector<GLuint> createTextureObjects(const tinygltf::Model &model) const;

  void computeTangentAndBitangentCoordinates(std::vector<glm::vec3> &tangents,
      std::vector<glm::vec3> &bitangents, std::vector<glm::vec3> &pos,
      std::vector<glm::vec2> &texCoord0);

  template <typename T>
  void computeData(const tinygltf::Model &model, const std::string dataName,
      std::vector<T> &container, const bool world = true);

  void computePos(const tinygltf::Model &model, std::vector<glm::vec3> &pos,
      bool world = true);
  void computeNormal(
      const tinygltf::Model &model, std::vector<glm::vec3> &normal);
  void computeTexCoord(
      const tinygltf::Model &model, std::vector<glm::vec2> &texCoord0);
};