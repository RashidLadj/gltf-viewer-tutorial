#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/FirstPersonCameraController.hpp"
#include "utils/TrackballCameraController.hpp"
#include "utils/cameraControllerInterface.hpp"
#include "utils/cameras.hpp"
#include <stb_image_write.h>
#include <tiny_gltf.h>

// Include for DrawNode --> calcul ModelMatrix
#include "utils/gltf.hpp"
#include "utils/images.hpp"

////////////////////////////////////////////////
/// OpenGL project
/// Ladjouli Rashid & Jarcet Eliot
////////////////////////////////////////////////

void keyCallback(
    GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, 1);
  }
}

int ViewerApplication::run()
{
  // Loader shaders
  const auto glslProgram =
      compileProgram({m_ShadersRootPath / m_AppName / m_vertexShader,
          m_ShadersRootPath / m_AppName / m_fragmentShader});

  const auto modelMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelMatrix");
  const auto modelViewProjMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelViewProjMatrix");
  const auto modelViewMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelViewMatrix");
  const auto normalMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uNormalMatrix");
  const auto lightDirectionLocation =
      glGetUniformLocation(glslProgram.glId(), "uLightDirection");
  const auto lightIntensityLocation =
      glGetUniformLocation(glslProgram.glId(), "uLightIntensity");
  const auto baseColorTextureLocation =
      glGetUniformLocation(glslProgram.glId(), "uBaseColorTexture");
  const auto baseColorFactorLocation =
      glGetUniformLocation(glslProgram.glId(), "uBaseColorFactor");

  const auto metallicFactorLocation =
      glGetUniformLocation(glslProgram.glId(), "uMetallicFactor");
  const auto metallicRoughnessTextureLocation =
      glGetUniformLocation(glslProgram.glId(), "uMetallicRoughnessTexture");
  const auto roughnessFactorLocation =
      glGetUniformLocation(glslProgram.glId(), "uRoughnessFactor");

  const auto emissiveFactorLocation =
      glGetUniformLocation(glslProgram.glId(), "uEmissiveFactor");
  const auto emissiveTextureLocation =
      glGetUniformLocation(glslProgram.glId(), "uEmissiveTexture");

  const auto normalTextureLocation =
      glGetUniformLocation(glslProgram.glId(), "uNormalTexture");

  tinygltf::Model model;
  // TODO Loading the glTF file
  if (!loadGltfFile(model)) {
    std::cout << " Can't Load GltfFile !!!" << std::endl;
    return false;
  }

  //  compute the bounding box of the scene
  glm::vec3 bboxMin, bboxMax;
  computeSceneBounds(model, bboxMin, bboxMax);

  // Diagonal vector ??
  const auto diagVector = bboxMax - bboxMin;

  // Build projection matrix
  // auto maxDistance = 500.f; // TODO use scene bounds instead to compute this
  float maxDistance =
      glm::length(diagVector) > 0 ? glm::length(diagVector) : 100.f;
  const auto projMatrix =
      glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight,
          0.001f * maxDistance, 1.5f * maxDistance);

  // TODO Implement a new CameraController model and use it instead. Propose the
  // choice from the GUI
  // FirstPersonCameraController cameraController{m_GLFWHandle.window(), 2.f *
  // maxDistance};
  /** Replace FirstPersonCamera With TrackballCamera**/
  // TrackballCameraController cameraController{m_GLFWHandle.window(), 2.f *
  // maxDistance};
  std::unique_ptr<CameraControllerInterface> cameraController =
      std::make_unique<TrackballCameraController>(m_GLFWHandle.window());

  if (m_hasUserCamera) {
    cameraController->setCamera(m_userCamera);
  } else {
    // TODO Use scene bounds to compute a better default camera
    // cameraController.setCamera(Camera{glm::vec3(0, 0, 0), glm::vec3(0, 0,
    // -1), glm::vec3(0, 1, 0)});
    const auto center = 0.5f * (bboxMax + bboxMin);
    const auto up = glm::vec3(0, 1, 0);
    // const auto eye = center + diag;
    // When handle flat scenes on the z axis
    const auto eye = diagVector.z > 0
                         ? center + diagVector
                         : center + 2.f * glm::cross(diagVector, up);
    cameraController->setCamera(Camera{eye, center, up});
  }

  auto lightDirection = glm::vec3(1.f);
  auto lightIntensity = glm::vec3(1.f);
  bool lightFromCamera = false;

  const std::vector<GLuint> textureObjects = createTextureObjects(model);
  float white[] = {1, 1, 1, 1};
  GLuint whiteTexture = 0;
  glGenTextures(1, &whiteTexture);
  glBindTexture(GL_TEXTURE_2D, whiteTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, white);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Creation of Buffer Objects
  size_t vertexNumber;

  const auto bufferObjects = createBufferObjects(model, vertexNumber);

  std::cout << "vertex number : " << vertexNumber << std::endl;
  // Creation of Vertex Array Objects
  std::vector<VaoRange> meshToVertexArrays;
  const auto vertexArrayObjects = createVertexArrayObjects(
      model, bufferObjects, meshToVertexArrays, vertexNumber);

  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);
  glslProgram.use();

  const auto bindMaterial = [&](const auto materialIndex) {
    if (materialIndex >= 0) {
      // only valid is materialIndex >= 0
      const auto &material = model.materials[materialIndex];
      const auto &pbrMetallicRoughness = material.pbrMetallicRoughness;

      if (baseColorTextureLocation >= 0) {
        auto textureObject = whiteTexture;
        const auto baseColorTextureIndex =
            pbrMetallicRoughness.baseColorTexture.index;
        if (baseColorTextureIndex >= 0) {
          // only valid if pbrMetallicRoughness.baseColorTexture.index >= 0:
          const tinygltf::Texture &texture =
              model.textures[baseColorTextureIndex];
          if (texture.source >= 0) {
            textureObject = textureObjects[texture.source];
          }
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(baseColorTextureLocation, 0);
      }

      if (baseColorFactorLocation >= 0) {
        glUniform4f(baseColorFactorLocation,
            (float)pbrMetallicRoughness.baseColorFactor[0],
            (float)pbrMetallicRoughness.baseColorFactor[1],
            (float)pbrMetallicRoughness.baseColorFactor[2],
            (float)pbrMetallicRoughness.baseColorFactor[3]);
      }

      if (metallicFactorLocation >= 0) {
        glUniform1f(
            metallicFactorLocation, (float)pbrMetallicRoughness.metallicFactor);
      }
      if (roughnessFactorLocation >= 0) {
        glUniform1f(roughnessFactorLocation,
            (float)pbrMetallicRoughness.roughnessFactor);
      }
      if (metallicRoughnessTextureLocation > 0) {
        auto textureObject = 0u;
        if (pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
          const auto &texture =
              model.textures[pbrMetallicRoughness.metallicRoughnessTexture
                                 .index];
          if (texture.source >= 0) {
            textureObject = textureObjects[texture.source];
          }
        }
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(metallicRoughnessTextureLocation, 1);
      }

      if (emissiveTextureLocation >= 0) {
        auto textureObject = textureObjects[material.emissiveTexture.index];
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(emissiveTextureLocation, 2);
      }
      if (emissiveFactorLocation >= 0) {
        auto emissiveFactor = material.emissiveFactor;
        glUniform3f(emissiveFactorLocation, (float)emissiveFactor[0],
            (float)emissiveFactor[1], (float)emissiveFactor[2]);
      }

      if (normalTextureLocation >= 0) {
        // NORMAL MAPPING //
        auto textureObject = textureObjects[material.normalTexture.index];
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(normalTextureLocation, 2);
      }

    } else {
      if (baseColorTextureLocation >= 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(baseColorTextureLocation, 0);
      }
      if (baseColorFactorLocation >= 0) {
        glUniform4f(baseColorFactorLocation, 1, 1, 1, 1);
      }
      if (metallicFactorLocation >= 0) {
        glUniform1f(metallicFactorLocation, 0.f);
      }
      if (roughnessFactorLocation >= 0) {
        glUniform1f(roughnessFactorLocation, 0.f);
      }
      if (metallicRoughnessTextureLocation >= 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(metallicRoughnessTextureLocation, 0);
      }
      if (emissiveTextureLocation >= 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(emissiveTextureLocation, 0);
      }
      if (metallicFactorLocation >= 0) {
        glUniform3f(metallicFactorLocation, 0, 0, 0);
      }

      if (normalTextureLocation >= 0) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(normalTextureLocation, 2);
      }
    }
  };

  /** Lambda function to draw the scene **/
  const auto drawScene = [&](const Camera &camera) {
    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto viewMatrix = camera.getViewMatrix();

    if (lightDirectionLocation >= 0) {
      if (lightFromCamera)
        glUniform3f(lightDirectionLocation, 0, 0, 1);
      else
        glUniform3fv(lightDirectionLocation, 1,
            glm::value_ptr(glm::normalize(
                glm::vec3(viewMatrix * glm::vec4(lightDirection, 0.)))));
    }
    if (lightIntensityLocation >= 0) {
      glUniform3fv(lightIntensityLocation, 1, glm::value_ptr(lightIntensity));
    }

    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          const tinygltf::Node &node = model.nodes[nodeIdx];
          const glm::mat4 modelMatrix =
              getLocalToWorldMatrix(node, parentMatrix);
          // si il a une mesh, nous recuperons l'indice
          if (node.mesh >= 0) {
            //  init  modelViewMatrix, modelViewProjectionMatrix, and
            //  normalMatrix
            const glm::mat4 MV = viewMatrix * modelMatrix;
            const glm::mat4 MVP = projMatrix * MV;
            const glm::mat4 N = glm::transpose(glm::inverse(MV));
            // Send all to Shaders
            glUniformMatrix4fv(
                modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(
                modelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(MV));
            glUniformMatrix4fv(
                modelViewProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(MVP));
            glUniformMatrix4fv(
                normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(N));

            /*********/
            // node.mesh = l'indice dans model.meshes
            const auto &mesh = model.meshes[node.mesh];
            const auto &vaoRange = meshToVertexArrays[node.mesh];
            // Nous recuperons ensuite les primitives a dessiner --> <
            // vaoRange.count
            for (size_t primitiveIndice = 0;
                 primitiveIndice < mesh.primitives.size(); ++primitiveIndice) {

              const auto vao =
                  vertexArrayObjects[vaoRange.begin + primitiveIndice];
              const auto &primitive = mesh.primitives[primitiveIndice];
              bindMaterial(primitive.material);
              glBindVertexArray(vao);
              if (primitive.indices >= 0) {
                const auto &accessor = model.accessors[primitive.indices];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto byteOffset =
                    accessor.byteOffset + bufferView.byteOffset;
                glDrawElements(primitive.mode, GLsizei(accessor.count),
                    accessor.componentType, (const GLvoid *)byteOffset);
              } else {
                const auto accessorIdx = (*begin(primitive.attributes)).second;
                const auto &accessor = model.accessors[accessorIdx];
                glDrawArrays(primitive.mode, 0, GLsizei(accessor.count));
              }
            }
            /*********/
          }
          // For Children Nodes
          for (const auto childNode : node.children) {
            drawNode(childNode, modelMatrix);
          }
        };

    // Draw the scene referenced by gltf file
    if (model.defaultScene >= 0) {
      // TODO Draw all nodes
      for (const auto nodeIndice : model.scenes[model.defaultScene].nodes) {
        drawNode(nodeIndice, glm::mat4(1));
      }
    }
  };

  // render in a Image
  if (!m_OutputPath.empty()) {
    std::vector<unsigned char> pixels(3 * m_nWindowWidth * m_nWindowHeight);
    renderToImage(m_nWindowWidth, m_nWindowHeight, 3, pixels.data(),
        [&]() { drawScene(cameraController->getCamera()); });
    flipImageYAxis(m_nWindowWidth, m_nWindowHeight, 3, pixels.data());
    const auto strPath = m_OutputPath.string();
    stbi_write_png(
        strPath.c_str(), m_nWindowWidth, m_nWindowHeight, 3, pixels.data(), 0);
    return 0;
  }

  // Loop until the user closes the window
  for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose();
       ++iterationCount) {
    const auto seconds = glfwGetTime();

    const auto camera = cameraController->getCamera();
    drawScene(camera);

    // GUI code:
    imguiNewFrame();

    {
      ImGui::Begin("GUI");
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
          1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("eye: %.3f %.3f %.3f", camera.eye().x, camera.eye().y,
            camera.eye().z);
        ImGui::Text("center: %.3f %.3f %.3f", camera.center().x,
            camera.center().y, camera.center().z);
        ImGui::Text(
            "up: %.3f %.3f %.3f", camera.up().x, camera.up().y, camera.up().z);

        ImGui::Text("front: %.3f %.3f %.3f", camera.front().x, camera.front().y,
            camera.front().z);
        ImGui::Text("left: %.3f %.3f %.3f", camera.left().x, camera.left().y,
            camera.left().z);

        if (ImGui::Button("CLI camera args to clipboard")) {
          std::stringstream ss;
          ss << "--lookat " << camera.eye().x << "," << camera.eye().y << ","
             << camera.eye().z << "," << camera.center().x << ","
             << camera.center().y << "," << camera.center().z << ","
             << camera.up().x << "," << camera.up().y << "," << camera.up().z;
          const auto str = ss.str();
          glfwSetClipboardString(m_GLFWHandle.window(), str.c_str());
        }

        /** 0->First   1-Trackball **/
        static int cameraControllerType = 0;
        const bool cameraControllerTypeChanged =
            ImGui::RadioButton("First Person", &cameraControllerType, 0) ||
            ImGui::RadioButton("Trackball", &cameraControllerType, 1);
        if (cameraControllerTypeChanged) {
          const Camera currentCamera = cameraController->getCamera();
          if (cameraControllerType == 0) {
            cameraController = std::make_unique<FirstPersonCameraController>(
                m_GLFWHandle.window(), 0.5f * maxDistance);
          } else {
            cameraController = std::make_unique<TrackballCameraController>(
                m_GLFWHandle.window());
          }
          cameraController->setCamera(currentCamera);
        }
        /** Parameter of Light**/
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
          static float theta = 0.f;
          static float phi = 0.f;

          if (ImGui::SliderFloat("Theta", &theta, 0, glm::pi<float>()) ||
              ImGui::SliderFloat("Phi", &phi, 0, 2.f * glm::pi<float>())) {
            const float sinPhi = glm::sin(phi);
            const float cosPhi = glm::cos(phi);
            const float sinTheta = glm::sin(theta);
            const float cosTheta = glm::cos(theta);
            lightDirection =
                glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
          }

          static auto lightColor = glm::vec3(1.f);
          static float lightIntensityFactor = 1.f;

          if (ImGui::SliderFloat("Intensity", &lightIntensityFactor, 0, 10.f) ||
              ImGui::ColorEdit3(
                  "color", reinterpret_cast<float *>(&lightColor))) {
            lightIntensity = lightColor * lightIntensityFactor;
          }
        }
        ImGui::Checkbox("Light from camera", &lightFromCamera);
      }
      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    auto ellapsedTime = glfwGetTime() - seconds;
    auto guiHasFocus =
        ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    if (!guiHasFocus) {
      cameraController->update(float(ellapsedTime));
    }

    m_GLFWHandle.swapBuffers(); // Swap front and back buffers
  }
  // TODO clean up allocated GL data

  return 0;
}

ViewerApplication::ViewerApplication(const fs::path &appPath, uint32_t width,
    uint32_t height, const fs::path &gltfFile,
    const std::vector<float> &lookatArgs, const std::string &vertexShader,
    const std::string &fragmentShader, const fs::path &output) :
    m_nWindowWidth(width),
    m_nWindowHeight(height),
    m_AppPath{appPath},
    m_AppName{m_AppPath.stem().string()},
    m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
    m_ShadersRootPath{m_AppPath.parent_path() / "shaders"},
    m_gltfFilePath{gltfFile},
    m_OutputPath{output}
{
  if (!lookatArgs.empty()) {
    m_hasUserCamera = true;
    m_userCamera =
        Camera{glm::vec3(lookatArgs[0], lookatArgs[1], lookatArgs[2]),
            glm::vec3(lookatArgs[3], lookatArgs[4], lookatArgs[5]),
            glm::vec3(lookatArgs[6], lookatArgs[7], lookatArgs[8])};
  }

  if (!vertexShader.empty()) {
    m_vertexShader = vertexShader;
  }

  if (!fragmentShader.empty()) {
    m_fragmentShader = fragmentShader;
  }

  ImGui::GetIO().IniFilename =
      m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows
                                  // positions in this file

  glfwSetKeyCallback(m_GLFWHandle.window(), keyCallback);

  printGLVersion();
}

bool ViewerApplication::loadGltfFile(tinygltf::Model &model)
{
  std::cout << "Loading glTF File ..." << std::endl;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  bool ret =
      loader.LoadASCIIFromFile(&model, &err, &warn, m_gltfFilePath.string());

  if (!warn.empty()) {
    printf("  Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("  Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("  Failed to parse glTF\n");
    return false;
  }
  return true;
}

std::vector<GLuint> ViewerApplication::createBufferObjects(
    const tinygltf::Model &model, size_t &vertexNumber)
{
  // create a vector of GLuint with the correct size (model.buffers.size()) and
  // use glGenBuffers to create buffer objects.
  std::vector<GLuint> bufferObjects(model.buffers.size(), 0);
  glGenBuffers(GLsizei(model.buffers.size()), bufferObjects.data());
  std::cout << "there is " << model.buffers.size()
            << " buffers in this gltf model" << std::endl;
  for (size_t i = 0; i < model.buffers.size(); ++i) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
    // glBufferStorage in 4.4, etant limité sur 4.3 je doit utiliser
    // glBufferData

    ////////////////////////////////////////////////////////////////
    ///         NORMAL MAPPING          ////////////////////////////
    ////////////////////////////////////////////////////////////////
    // on va ajouter les données : tangentes et bitangentes au buffer
    // le buffer actuel est structuré comme tel : [ [data???] [POSITION]
    // [NORMAL] [TEX_COORD_0] ]

    // on veut ajouter [TANGENT] et [BITANGENT] qui doivent chacune faire la
    // même taille que [POSITION] ou [NORMAL] puisqu'ils contiennent tous
    // 3 uchar par vertex (contrairement à TEX_COORD_0 qui n'en contient que 2)

    std::vector<unsigned char> augmentedBuffer = model.buffers[i].data;

    // ICI j'ai essayé de réecrire le buffer mais avec des floats
    // std::vector<float> buffer;

    // std::vector<glm::vec3> pos;
    /* = {
          glm::vec3(0.5, 0.5, 0), glm::vec3(0.5, -0.5, 0), glm::vec3(-0.8,
       0, 0)};*/
    // std::vector<glm::vec3> normal;
    /* = { glm::vec3(0, 0, 1), glm::vec3(0,
    // 0,
         1), glm::vec3(0, 0, 1)
  };
  * /
      // std::vector<glm::vec2> texCoord0; /* = {
      glm::vec2(0.5, 0.5),
      glm::vec2(0.5, -0.5), glm::vec2(-0.8, 0)
};
* /

    computePos(model, pos);
computeNormal(model, normal);
computeTexCoord(model, texCoord0);
std::cout << "pos size : " << pos.size() << std::endl;
std::cout << "normal size : " << normal.size() << std::endl;
std::cout << "texCoord0 size : " << texCoord0.size() << std::endl;

vertexNumber = pos.size();

// Computing tangent and bitangent

std::vector<glm::vec3> tangents;
std::vector<glm::vec3> bitangents;

computeTangentAndBitangentCoordinates(tangents, bitangents, pos, texCoord0);

/*
    for (int i = 0; i < pos.size(); ++i) {
      buffer.push_back(pos[i].x);
      buffer.push_back(pos[i].y);
      buffer.push_back(pos[i].z);
      buffer.push_back(normal[i].x);
      buffer.push_back(normal[i].y);
      buffer.push_back(normal[i].z);
      buffer.push_back(texCoord0[i].x);
      buffer.push_back(texCoord0[i].y);
      buffer.push_back(tangents[i].x);
      buffer.push_back(tangents[i].y);
      buffer.push_back(tangents[i].z);
      buffer.push_back(bitangents[i].x);
      buffer.push_back(bitangents[i].y);
      buffer.push_back(bitangents[i].z);
    }*/
    /*
        for (auto p : pos) {
          buffer.push_back(p.x);
          buffer.push_back(p.y);
          buffer.push_back(p.z);
        }
        for (auto n : normal) {
          buffer.push_back(n.x);
          buffer.push_back(n.y);
          buffer.push_back(n.z);
        }
        for (auto tex : texCoord0) {
          buffer.push_back(tex.x);
          buffer.push_back(tex.y);
        }
        for (auto t : tangents) {
          buffer.push_back(t.x);
          buffer.push_back(t.y);
          buffer.push_back(t.z);
        }
        for (auto b : bitangents) {
          buffer.push_back(b.x);
          buffer.push_back(b.y);
          buffer.push_back(b.z);
        }*/

    // combien de uchar rajouter ? La différence de byteOffset entre
    //[NORMAL]
    // et [POSITION] qui ont les accessorIdx respectifs : 1 et 2

    const auto &accessor1 = model.accessors[1];
    const auto &bufferView1 = model.bufferViews[accessor1.bufferView];

    const auto &accessor2 = model.accessors[2];
    const auto &bufferView2 = model.bufferViews[accessor2.bufferView];

    size_t sizeForTangent = bufferView2.byteOffset - bufferView1.byteOffset;

    // filling buffer with bitangent data

    for (int i = 0; i < sizeForTangent; ++i) {
      augmentedBuffer.push_back(62);
    }

    // filling buffer with bitangent data
    for (int i = 0; i < sizeForTangent; ++i) {
      augmentedBuffer.push_back(0);
    }

    // On donne le nouveau buffer au GPU
    glBufferData(GL_ARRAY_BUFFER, augmentedBuffer.size(),
        augmentedBuffer.data(), GL_STATIC_DRAW);
    // glBufferData(GL_ARRAY_BUFFER, buffer.size(), buffer.data(),
    // GL_STATIC_DRAW);

    ////////////////////////////////////////////////////////////////
    ///         \NORMAL MAPPING          ///////////////////////////
    ////////////////////////////////////////////////////////////////
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return bufferObjects;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects(
    const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects,
    std::vector<VaoRange> &meshIndexToVaoRange, size_t vertexNumber)
{
  std::vector<GLuint> vertexArrayObjects;
  const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
  const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
  const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;
  const GLuint VERTEX_ATTRIB_TANGENT_IDX = 3;
  const GLuint VERTEX_ATTRIB_BITANGENT_IDX = 4;

  // { Indice of Mesh , Number of primitives}  need this to Draw After
  meshIndexToVaoRange.resize(model.meshes.size());

  // const auto vaoOffset = vertexArrayObjects.size();
  // vertexArrayObjects.resize(vaoOffset +
  // model.meshes[meshIdx].primitives.size());
  // meshIndexToVaoRange.push_back(VaoRange{vaoOffset,
  // model.meshes[meshIdx].primitives.size()});

  GLsizei compteur = 0;
  for (const auto &mesh : model.meshes) {

    auto vaoOffset = GLsizei(vertexArrayObjects.size());
    meshIndexToVaoRange[compteur].begin = vaoOffset;
    auto numberOfPrimitives = GLsizei(mesh.primitives.size());
    meshIndexToVaoRange[compteur].count = numberOfPrimitives;

    // resize vector of VAOs en ajoutant a chque iteration le nombre de
    // primitives de la "Mesh"
    vertexArrayObjects.resize(
        vertexArrayObjects.size() + mesh.primitives.size());

    std::cout << "number of primitives :" << numberOfPrimitives << std::endl;

    // vaoOffset represente le debut du poiteur sur le tableau et
    // numberOfPrimitive la taille
    glGenVertexArrays(numberOfPrimitives, &vertexArrayObjects[vaoOffset]);

    for (size_t pimitiveIndice = 0; pimitiveIndice < size_t(numberOfPrimitives);
         ++pimitiveIndice) {

      const auto vao = vertexArrayObjects[vaoOffset + pimitiveIndice];
      const auto &primitive = mesh.primitives[pimitiveIndice];
      glBindVertexArray(vao);

      { // I'm opening a scope because I want to reuse the variable
        // iterator
        // in the code for NORMAL and TEXCOORD_0
        // const std::string parameters[] = {"POSITION", "NORMAL",
        // "TEXCOORD_0"};
        const GLuint parametersVertexAttribs[] = {VERTEX_ATTRIB_POSITION_IDX,
            VERTEX_ATTRIB_NORMAL_IDX, VERTEX_ATTRIB_TEXCOORD0_IDX,
            VERTEX_ATTRIB_TANGENT_IDX, VERTEX_ATTRIB_BITANGENT_IDX};
        std::map<int, std::string> mymap;
        // ICI l'orghographe et la syntaxe MAJUSCULE des attribus est trés
        // importants! ils doivent correspondre à celle du model glTF
        mymap[VERTEX_ATTRIB_POSITION_IDX] = "POSITION";
        mymap[VERTEX_ATTRIB_NORMAL_IDX] = "NORMAL";
        mymap[VERTEX_ATTRIB_TEXCOORD0_IDX] = "TEXCOORD_0";
        mymap[VERTEX_ATTRIB_TANGENT_IDX] = "TANGENT";
        mymap[VERTEX_ATTRIB_BITANGENT_IDX] = "BITANGENT";

        glBindBuffer(GL_ARRAY_BUFFER, 1);

        size_t globalByteOffset;
        for (const GLuint vertexAttrib : parametersVertexAttribs) {
          // std::cout <<  "Nous traitons l'attribut " <<
          // mymap.find(vertexAttrib)->second << std::endl;

          // this part of the code (with the iterator) manages the vertex
          // attributes present in the primitive attributes description
          // (Position, Normal, Texcoorrd_0)
          const auto iterator =
              primitive.attributes.find(mymap.find(vertexAttrib)->second);

          if (iterator != end(primitive.attributes)) {

            glEnableVertexAttribArray(vertexAttrib);

            const auto accessorIdx = (*iterator).second;

            const auto &accessor = model.accessors[accessorIdx];
            const auto &bufferView = model.bufferViews[accessor.bufferView];
            const auto bufferIdx = bufferView.buffer;
            const auto bufferObject = bufferObjects[bufferIdx];
            // c'est un indice

            assert(GL_ARRAY_BUFFER == bufferView.target);

            // ICI on bind le buffer dans un loop sur les vertex
            // attributes
            // C'est redondant : dans les models glTF tout est stoqué dans
            // un
            // seul et même buffer! il est donc "bindé" 3 fois mais on
            // pourrait le binder qu'une seule fois avant le loop ça
            // marche
            // glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

            const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;

            std::cout << GLsizei(bufferView.byteStride) << std::endl
                      << byteOffset << std::endl
                      << std::endl;
            glVertexAttribPointer(vertexAttrib, accessor.type,
                accessor.componentType, GL_FALSE,
                GLsizei(bufferView.byteStride), (const GLvoid *)byteOffset);

            globalByteOffset = byteOffset;
          }
        }
        ////////////////////////////////////////////////////////////////
        ///         NORMAL MAPPING          ////////////////////////////
        ////////////////////////////////////////////////////////////////

        // this part : i manage the vertex attributes which are not
        // present in
        // the primitive attributes description : Tangent & Bitangent

        // whats the size of tangent & bitangent data ?

        const auto &accessor1 = model.accessors[1];
        const auto &bufferView1 = model.bufferViews[accessor1.bufferView];

        const auto &accessor2 = model.accessors[2];
        const auto &bufferView2 = model.bufferViews[accessor2.bufferView];

        size_t sizeForTangent = bufferView2.byteOffset - bufferView1.byteOffset;

        size_t sizeForTexCoord = sizeForTangent * 2 / 3;

        // we need to increment the offset by the number of data
        // stored for  TexCoord_0
        globalByteOffset += sizeForTexCoord;

        std::cout << "Telling vao how to manage tangent data" << std::endl;
        glEnableVertexAttribArray(VERTEX_ATTRIB_TANGENT_IDX);
        glVertexAttribPointer(VERTEX_ATTRIB_TANGENT_IDX, 3, GL_FLOAT, GL_FALSE,
            0, (const GLvoid *)globalByteOffset);

        globalByteOffset += sizeForTangent;

        std::cout << "Telling vao how to manage bitangent data" << std::endl;
        glEnableVertexAttribArray(VERTEX_ATTRIB_BITANGENT_IDX);
        glVertexAttribPointer(VERTEX_ATTRIB_BITANGENT_IDX, 3, GL_FLOAT,
            GL_FALSE, 0, (const GLvoid *)globalByteOffset);

        ////////////////////////////////////////////////////////////////
        ///         \NORMAL MAPPING          ///////////////////////////
        ////////////////////////////////////////////////////////////////

        // ??
        // Ajout aprés oublie
        if (primitive.indices >= 0) {
          const auto accessorIdx = primitive.indices;
          const auto &accessor = model.accessors[accessorIdx];
          const auto &bufferView = model.bufferViews[accessor.bufferView];
          const auto bufferIdx = bufferView.buffer;
          // Correction de l'assert
          assert(GL_ELEMENT_ARRAY_BUFFER == bufferView.target);
          const auto bufferObject = bufferObjects[bufferIdx];
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject);
        }

        // on débind le buffer (vbo)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      }

      /*
            glBindBuffer(GL_ARRAY_BUFFER, 1);

            // size_t byteOffset = 0;

            int vertexStride = (3 + 3 + 2 + 3 + 3) * sizeof(float);
            GLintptr vertex_position_offset = 0 * sizeof(float);
            GLintptr vertex_normal_offset = 3 * sizeof(float);
            GLintptr vertex_texcoord_offset = 6 * sizeof(float);
            GLintptr vertex_tangent_offset = 8 * sizeof(float);
            GLintptr vertex_bitangent_offset = 11 * sizeof(float);

            glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION_IDX);
            glVertexAttribPointer(VERTEX_ATTRIB_POSITION_IDX, 3, GL_FLOAT,
         GL_FALSE, vertexStride, (const GLvoid *)vertex_position_offset);
            // byteOffset += vertexNumber * 3 * sizeof(float);

            glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL_IDX);
            glVertexAttribPointer(VERTEX_ATTRIB_POSITION_IDX, 3, GL_FLOAT,
         GL_FALSE, vertexStride, (const GLvoid *)vertex_normal_offset);
            // byteOffset += vertexNumber * 3 * sizeof(float);

            glEnableVertexAttribArray(VERTEX_ATTRIB_NORMAL_IDX);
            glVertexAttribPointer(VERTEX_ATTRIB_TEXCOORD0_IDX, 2, GL_FLOAT,
         GL_FALSE, vertexStride, (const GLvoid *)vertex_texcoord_offset);
            // byteOffset += vertexNumber * 2 * sizeof(float);

            glEnableVertexAttribArray(VERTEX_ATTRIB_TANGENT_IDX);
            glVertexAttribPointer(VERTEX_ATTRIB_TANGENT_IDX, 3, GL_FLOAT,
         GL_FALSE, vertexStride, (const GLvoid *)vertex_tangent_offset);
            // byteOffset += vertexNumber * 3 * sizeof(float);

            glEnableVertexAttribArray(VERTEX_ATTRIB_BITANGENT_IDX);
            glVertexAttribPointer(VERTEX_ATTRIB_BITANGENT_IDX, 3, GL_FLOAT,
         GL_FALSE, vertexStride, (const GLvoid *)vertex_bitangent_offset);

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            */
    }
  }
  compteur++;

  // on debind le vao
  glBindVertexArray(0);
  return vertexArrayObjects;
}

std::vector<GLuint> ViewerApplication::createTextureObjects(
    const tinygltf::Model &model) const
{
  std::vector<GLuint> textureObjects(model.textures.size(), 0);

  /** Definition Default Simpler dans le cas ou ils ne sont pas definit dans
   * le model **/
  tinygltf::Sampler defaultSampler;
  defaultSampler.minFilter = GL_LINEAR;
  defaultSampler.magFilter = GL_LINEAR;
  defaultSampler.wrapS = GL_REPEAT;
  defaultSampler.wrapT = GL_REPEAT;
  defaultSampler.wrapR = GL_REPEAT;

  glActiveTexture(GL_TEXTURE0);
  glGenTextures(GLsizei(model.textures.size()), textureObjects.data());

  for (size_t i = 0; i < model.textures.size(); ++i) {
    const auto &texture = model.textures[i];
    assert(texture.source >= 0); // ensure a source image is present
    const auto &image = model.images[texture.source];
    const auto &sampler =
        texture.sampler >= 0 ? model.samplers[texture.sampler] : defaultSampler;
    glBindTexture(GL_TEXTURE_2D, textureObjects[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
        GL_RGBA, image.pixel_type, image.image.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        sampler.minFilter != -1 ? sampler.minFilter : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        sampler.magFilter != -1 ? sampler.magFilter : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, sampler.wrapR);

    if (sampler.minFilter == GL_NEAREST_MIPMAP_NEAREST ||
        sampler.minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        sampler.minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        sampler.minFilter == GL_LINEAR_MIPMAP_LINEAR) {
      glGenerateMipmap(GL_TEXTURE_2D);
    }
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  return textureObjects;
}

void ViewerApplication::computeTangentAndBitangentCoordinates(
    std::vector<glm::vec3> &tangents, std::vector<glm::vec3> &bitangents,
    std::vector<glm::vec3> &pos, std::vector<glm::vec2> &texCoord0)
{
  glm::vec3 tangent;
  glm::vec3 bitangent;
  for (int i = 0; i < pos.size(); i += 3) {
    // Shortcuts for vertices
    glm::vec3 &v0 = pos[i + 0];
    glm::vec3 &v1 = pos[i + 1];
    glm::vec3 &v2 = pos[i + 2];

    // Shortcuts for UVs
    glm::vec2 &uv0 = texCoord0[i + 0];
    glm::vec2 &uv1 = texCoord0[i + 1];
    glm::vec2 &uv2 = texCoord0[i + 2];

    // Edges of the triangle : position delta
    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;

    // UV delta
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;

    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

    // Set the same tangent for all three vertices of the triangle.
    // They will be merged later, in vboindexer.cpp
    tangents.push_back(tangent);
    tangents.push_back(tangent);
    tangents.push_back(tangent);

    // Same thing for bitangents
    bitangents.push_back(bitangent);
    bitangents.push_back(bitangent);
    bitangents.push_back(bitangent);
  }
}

void ViewerApplication::computePos(
    const tinygltf::Model &model, std::vector<glm::vec3> &pos)
{
  if (model.defaultScene >= 0) {
    const std::function<void(int, const glm::mat4 &)> addPos =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          const auto &node = model.nodes[nodeIdx];
          const glm::mat4 modelMatrix =
              getLocalToWorldMatrix(node, parentMatrix);
          if (node.mesh >= 0) {
            const auto &mesh = model.meshes[node.mesh];
            for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {

              const auto &primitive = mesh.primitives[pIdx];
              const auto positionAttrIdxIt =
                  primitive.attributes.find("POSITION");
              if (positionAttrIdxIt == end(primitive.attributes)) {
                continue;
              }
              const auto &positionAccessor =
                  model.accessors[(*positionAttrIdxIt).second];
              if (positionAccessor.type != 3) {
                std::cerr << "Position accessor with type != VEC3, skipping"
                          << std::endl;
                continue;
              }
              const auto &positionBufferView =
                  model.bufferViews[positionAccessor.bufferView];
              const auto byteOffset =
                  positionAccessor.byteOffset + positionBufferView.byteOffset;
              const auto &positionBuffer =
                  model.buffers[positionBufferView.buffer];
              const auto positionByteStride =
                  positionBufferView.byteStride ? positionBufferView.byteStride
                                                : 3 * sizeof(float);

              if (primitive.indices >= 0) {
                const auto &indexAccessor = model.accessors[primitive.indices];
                const auto &indexBufferView =
                    model.bufferViews[indexAccessor.bufferView];
                const auto indexByteOffset =
                    indexAccessor.byteOffset + indexBufferView.byteOffset;
                const auto &indexBuffer = model.buffers[indexBufferView.buffer];
                auto indexByteStride = indexBufferView.byteStride;

                switch (indexAccessor.componentType) {
                default:
                  std::cerr
                      << "Primitive index accessor with bad componentType "
                      << indexAccessor.componentType << ", skipping it."
                      << std::endl;
                  continue;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint8_t);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint16_t);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint32_t);
                  break;
                }

                for (size_t i = 0; i < indexAccessor.count; ++i) {
                  uint32_t index = 0;
                  switch (indexAccessor.componentType) {
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index = *((const uint8_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index = *((const uint16_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index = *((const uint32_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  }
                  const auto &localPosition =
                      *((const glm::vec3 *)&positionBuffer
                              .data[byteOffset + positionByteStride * index]);
                  const auto worldPosition =
                      glm::vec3(modelMatrix * glm::vec4(localPosition, 1.f));

                  pos.push_back(worldPosition);
                }
              } else {
                for (size_t i = 0; i < positionAccessor.count; ++i) {
                  const auto &localPosition =
                      *((const glm::vec3 *)&positionBuffer
                              .data[byteOffset + positionByteStride * i]);
                  const auto worldPosition =
                      glm::vec3(modelMatrix * glm::vec4(localPosition, 1.f));

                  pos.push_back(worldPosition);
                }
              }
            }
          }
          for (const auto childNodeIdx : node.children) {
            addPos(childNodeIdx, modelMatrix);
          }
        };
    for (const auto nodeIdx : model.scenes[model.defaultScene].nodes) {
      addPos(nodeIdx, glm::mat4(1));
    }
  }
}

void ViewerApplication::computeNormal(
    const tinygltf::Model &model, std::vector<glm::vec3> &normal)
{
  if (model.defaultScene >= 0) {
    const std::function<void(int, const glm::mat4 &)> addNormal =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          const auto &node = model.nodes[nodeIdx];
          const glm::mat4 modelMatrix =
              getLocalToWorldMatrix(node, parentMatrix);
          if (node.mesh >= 0) {
            const auto &mesh = model.meshes[node.mesh];
            for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {

              const auto &primitive = mesh.primitives[pIdx];
              const auto normalAttrIdxIt = primitive.attributes.find("NORMAL");
              if (normalAttrIdxIt == end(primitive.attributes)) {
                continue;
              }
              const auto &normalAccessor =
                  model.accessors[(*normalAttrIdxIt).second];
              if (normalAccessor.type != 3) {
                std::cerr << "normal accessor with type != VEC3, skipping"
                          << std::endl;
                continue;
              }
              const auto &normalBufferView =
                  model.bufferViews[normalAccessor.bufferView];
              const auto byteOffset =
                  normalAccessor.byteOffset + normalBufferView.byteOffset;
              const auto &normalBuffer = model.buffers[normalBufferView.buffer];
              const auto normalByteStride = normalBufferView.byteStride
                                                ? normalBufferView.byteStride
                                                : 3 * sizeof(float);

              if (primitive.indices >= 0) {
                const auto &indexAccessor = model.accessors[primitive.indices];
                const auto &indexBufferView =
                    model.bufferViews[indexAccessor.bufferView];
                const auto indexByteOffset =
                    indexAccessor.byteOffset + indexBufferView.byteOffset;
                const auto &indexBuffer = model.buffers[indexBufferView.buffer];
                auto indexByteStride = indexBufferView.byteStride;

                switch (indexAccessor.componentType) {
                default:
                  std::cerr
                      << "Primitive index accessor with bad componentType "
                      << indexAccessor.componentType << ", skipping it."
                      << std::endl;
                  continue;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint8_t);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint16_t);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint32_t);
                  break;
                }

                for (size_t i = 0; i < indexAccessor.count; ++i) {
                  uint32_t index = 0;
                  switch (indexAccessor.componentType) {
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index = *((const uint8_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index = *((const uint16_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index = *((const uint32_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  }
                  const auto &localNormal =
                      *((const glm::vec3 *)&normalBuffer
                              .data[byteOffset + normalByteStride * index]);
                  const auto worldNormal =
                      glm::vec3(modelMatrix * glm::vec4(localNormal, 1.f));

                  normal.push_back(worldNormal);
                }
              } else {
                for (size_t i = 0; i < normalAccessor.count; ++i) {
                  const auto &localNormal =
                      *((const glm::vec3 *)&normalBuffer
                              .data[byteOffset + normalByteStride * i]);
                  const auto worldNormal =
                      glm::vec3(modelMatrix * glm::vec4(localNormal, 1.f));

                  normal.push_back(worldNormal);
                }
              }
            }
          }
          for (const auto childNodeIdx : node.children) {
            addNormal(childNodeIdx, modelMatrix);
          }
        };
    for (const auto nodeIdx : model.scenes[model.defaultScene].nodes) {
      addNormal(nodeIdx, glm::mat4(1));
    }
  }
}

void ViewerApplication::computeTexCoord(
    const tinygltf::Model &model, std::vector<glm::vec2> &texCoord0)
{
  if (model.defaultScene >= 0) {
    const std::function<void(int, const glm::mat4 &)> addTexCoord =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          const auto &node = model.nodes[nodeIdx];
          const glm::mat4 modelMatrix =
              getLocalToWorldMatrix(node, parentMatrix);
          if (node.mesh >= 0) {
            const auto &mesh = model.meshes[node.mesh];
            for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {

              const auto &primitive = mesh.primitives[pIdx];
              const auto texCoordAttrIdxIt =
                  primitive.attributes.find("TEXCOORD_0");
              if (texCoordAttrIdxIt == end(primitive.attributes)) {
                continue;
              }
              const auto &texCoordAccessor =
                  model.accessors[(*texCoordAttrIdxIt).second];
              if (texCoordAccessor.type != 2) {
                std::cerr << "texCoord accessor with type != VEC2, skipping"
                          << std::endl;
                continue;
              }
              const auto &texCoordBufferView =
                  model.bufferViews[texCoordAccessor.bufferView];
              const auto byteOffset =
                  texCoordAccessor.byteOffset + texCoordBufferView.byteOffset;
              const auto &texCoordBuffer =
                  model.buffers[texCoordBufferView.buffer];
              const auto texCoordByteStride =
                  texCoordBufferView.byteStride ? texCoordBufferView.byteStride
                                                : 2 * sizeof(float);

              if (primitive.indices >= 0) {
                const auto &indexAccessor = model.accessors[primitive.indices];
                const auto &indexBufferView =
                    model.bufferViews[indexAccessor.bufferView];
                const auto indexByteOffset =
                    indexAccessor.byteOffset + indexBufferView.byteOffset;
                const auto &indexBuffer = model.buffers[indexBufferView.buffer];
                auto indexByteStride = indexBufferView.byteStride;

                switch (indexAccessor.componentType) {
                default:
                  std::cerr
                      << "Primitive index accessor with bad componentType "
                      << indexAccessor.componentType << ", skipping it."
                      << std::endl;
                  continue;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint8_t);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint16_t);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                  indexByteStride =
                      indexByteStride ? indexByteStride : sizeof(uint32_t);
                  break;
                }

                for (size_t i = 0; i < indexAccessor.count; ++i) {
                  uint32_t index = 0;
                  switch (indexAccessor.componentType) {
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index = *((const uint8_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index = *((const uint16_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index = *((const uint32_t *)&indexBuffer
                                  .data[indexByteOffset + indexByteStride * i]);
                    break;
                  }

                  // ICI VEC2 PAS VEC3; FAUT IL FAIRE WORLD OU LOCAL COORD ?
                  const auto &localTexCoord =
                      *((const glm::vec2 *)&texCoordBuffer
                              .data[byteOffset + texCoordByteStride * index]);
                  /*const auto worldTexCoord =
                      glm::vec2(modelMatrix * glm::vec4(localTexCoord,
                     0, 1.f));
                  */
                  texCoord0.push_back(localTexCoord);
                }
              } else {
                for (size_t i = 0; i < texCoordAccessor.count; ++i) {
                  const auto &localTexCoord =
                      *((const glm::vec2 *)&texCoordBuffer
                              .data[byteOffset + texCoordByteStride * i]);
                  /*const auto worldTexCoord =
                      glm::vec2(modelMatrix *
                     glm::vec4(localTexCoord, 1.f));
                  */
                  texCoord0.push_back(localTexCoord);
                }
              }
            }
          }
          for (const auto childNodeIdx : node.children) {
            addTexCoord(childNodeIdx, modelMatrix);
          }
        };
    for (const auto nodeIdx : model.scenes[model.defaultScene].nodes) {
      addTexCoord(nodeIdx, glm::mat4(1));
    }
  }
}