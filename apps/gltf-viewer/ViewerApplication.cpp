/** Branch Many Lights Test **/
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
/// Ladjouzi Rachid & Jarcet Eliot
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

  const auto modelViewProjMatrix =
      glGetUniformLocation(glslProgram.glId(), "uModelViewProjMatrix");
  const auto modelViewMatrix =
      glGetUniformLocation(glslProgram.glId(), "uModelViewMatrix");
  const auto normalMatrix =
      glGetUniformLocation(glslProgram.glId(), "uNormalMatrix");

  /** Directional Light **/
  const auto uLightDirection =
      glGetUniformLocation(glslProgram.glId(), "dirLight.uLightDirection");
  const auto uLightIntensity =
      glGetUniformLocation(glslProgram.glId(), "dirLight.uLightIntensity");

  /** Point light **/
  const auto uPointLightPosition =
      glGetUniformLocation(glslProgram.glId(), "pointLight.position");
  const auto uPointLightColor =
      glGetUniformLocation(glslProgram.glId(), "pointLight.color");
  const auto uPointLightConstant =
      glGetUniformLocation(glslProgram.glId(), "pointLight.constant");
  const auto uPointLightQuadratic =
      glGetUniformLocation(glslProgram.glId(), "pointLight.quadratic");
  const auto uPointLightLinear =
      glGetUniformLocation(glslProgram.glId(), "pointLight.linear");

  /** Spot light **/
  const auto uSpotLightPosition =
      glGetUniformLocation(glslProgram.glId(), "spotLight.position");
  const auto uSpotLightDirection =
      glGetUniformLocation(glslProgram.glId(), "spotLight.direction");
  const auto uSpotLightColor =
      glGetUniformLocation(glslProgram.glId(), "spotLight.color");
  const auto uSpotLightCutOff =
      glGetUniformLocation(glslProgram.glId(), "spotLight.cutOff");
  const auto uSpotLightOuterCutOff =
      glGetUniformLocation(glslProgram.glId(), "spotLight.outerCutOff");
  const auto uSpotLightConstant =
      glGetUniformLocation(glslProgram.glId(), "spotLight.constant");
  const auto uSpotLightLinear =
      glGetUniformLocation(glslProgram.glId(), "spotLight.linear");
  const auto uSpotLightQuadratic =
      glGetUniformLocation(glslProgram.glId(), "spotLight.quadratic");

  const auto uBaseColorTexture =
      glGetUniformLocation(glslProgram.glId(), "uBaseColorTexture");
  const auto uBaseColorFactor =
      glGetUniformLocation(glslProgram.glId(), "uBaseColorFactor");

  const auto uMetallicFactor =
      glGetUniformLocation(glslProgram.glId(), "uMetallicFactor");
  const auto uMetallicRoughnessTexture =
      glGetUniformLocation(glslProgram.glId(), "uMetallicRoughnessTexture");
  const auto uRoughnessFactor =
      glGetUniformLocation(glslProgram.glId(), "uRoughnessFactor");

  const auto uEmissiveFactor =
      glGetUniformLocation(glslProgram.glId(), "uEmissiveFactor");
  const auto uEmissiveTexture =
      glGetUniformLocation(glslProgram.glId(), "uEmissiveTexture");

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

  /** Directional Light **/
  auto lightDirection = glm::vec3(1.f);
  auto lightIntensity = glm::vec3(1.f);
  bool lightFromCamera = false;

  /** Point Light **/
  auto pointLightIntensity = glm::vec3(1.f);
  bool enablePointLight = true;
  auto pointLightPosition = glm::vec3(-10.f, 5.f, 0.f);

  /** Spot Light **/
  auto spotLightIntensity = glm::vec3(1.f);
  auto spotLightPosition = glm::vec3(0.f);
  auto spotLightDirection = glm::vec3(0.f, 0.f, -1.f);
  bool enableSpotLight = true;
  float spotLightCutOff = 12.5f;
  float spotLightOuterCutOff = 17.5f;

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

  // TODO Creation of Buffer Objects
  const auto bufferObjects = createBufferObjects(model);

  // TODO Creation of Vertex Array Objects
  std::vector<VaoRange> meshToVertexArrays;
  const auto vertexArrayObjects =
      createVertexArrayObjects(model, bufferObjects, meshToVertexArrays);

  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);
  glslProgram.use();

  /** Lambda function to draw the scene **/

  // Old bindTexture, there is a pb inside --> to debug

  /*
  const auto bindMaterial = [&](const auto materialIndex) {
    // Material binding
    if(materialIndex>=0){
      // only valid is materialIndex >= 0
      const auto &material = model.materials[materialIndex];
      const auto &pbrMetallicRoughness = material.pbrMetallicRoughness;

       if (uBaseColorTexture >= 0) {
          auto textureObject = whiteTexture;
          const auto baseColorTextureIndex =
  pbrMetallicRoughness.baseColorTexture.index; if (baseColorTextureIndex >= 0) {
            // only valid if pbrMetallicRoughness.baseColorTexture.index >= 0:
            const tinygltf::Texture &texture =
  model.textures[baseColorTextureIndex]; if (texture.source >= 0) {
                textureObject = textureObjects[texture.source];
            }
          }
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, textureObject);
          glUniform1i(uBaseColorTexture, 0);
      }
      if (uBaseColorFactor >= 0) {
        // Convertion impossible de double en float -> so je passe à la formule
  qui est chiante
        //glUniform4fv(uBaseColorFactor, 1,
  glm::value_ptr(pbrMetallicRoughness.baseColorFactor)); glm::vec4
  baseColorfactor =glm::vec4(float(pbrMetallicRoughness.baseColorFactor[0]),
                float(pbrMetallicRoughness.baseColorFactor[1]),
                float(pbrMetallicRoughness.baseColorFactor[2]),
                float(pbrMetallicRoughness.baseColorFactor[3])
        );
        glUniform4fv(uBaseColorFactor, 1, glm::value_ptr(baseColorfactor));
      }
      if (uMetallicRoughnessTexture >= 0) {
        GLuint textureObject = 0;
        const auto pbrMetallicRoughnessIndex =
  pbrMetallicRoughness.metallicRoughnessTexture.index; if
  (pbrMetallicRoughnessIndex >= 0) { const tinygltf::Texture &texture =
  model.textures[pbrMetallicRoughnessIndex]; if (texture.source >= 0) {
                textureObject = textureObjects[texture.source];
            }
        }
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(uMetallicRoughnessTexture, 1);
      }
      if (uMetallicFactor >= 0) {
        glUniform1f(uMetallicFactor,
  float(pbrMetallicRoughness.metallicFactor));
      }
      if (uEmissiveFactor >= 0) {
        glm::vec3 emissiveFact =glm::vec3(float(material.emissiveFactor[0]),
                float(material.emissiveFactor[1]),
                float(material.emissiveFactor[2])
        );
        glUniform3fv(uEmissiveFactor, 1, glm::value_ptr(emissiveFact));
      }
      if (uEmissiveTexture >= 0) {
        GLuint textureObject = 0;
        const auto EmissiveIndex = material.emissiveTexture.index;
        if (EmissiveIndex >= 0) {
            const tinygltf::Texture &texture = model.textures[EmissiveIndex];
            if (texture.source >= 0) {
                textureObject = textureObjects[texture.source];
            }
        }
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(uMetallicRoughnessTexture, 2);
      }
    }
    else{
      if (uBaseColorTexture >= 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(uBaseColorTexture, 0);
      }
      if (uBaseColorFactor >= 0) {
        if (uBaseColorFactor >= 0) {
          glUniform4f(uBaseColorFactor, 1, 1, 1, 1);
        }
      }
      if (uMetallicRoughnessTexture >= 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(uMetallicRoughnessTexture, 1);
      }
      if (uMetallicFactor >= 0) {
          glUniform1f(uMetallicFactor, 1.f);
      }
      if (uRoughnessFactor >= 0) {
          glUniform1f(uRoughnessFactor, 1.f);
      }
      if (uEmissiveFactor >= 0) {
        glm::vec3 emissiveFact =glm::vec3(1.f);
        glUniform3fv(uEmissiveFactor, 1, glm::value_ptr(emissiveFact));
      }
      if (uEmissiveTexture >= 0) {
         glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(uMetallicRoughnessTexture, 2);
      }
    }
*/

  // Eliot's bindMaterial(), not the best architecture but working
  const auto bindMaterial = [&](const auto materialIndex) {
    if (materialIndex >= 0) {
      // only valid is materialIndex >= 0
      const auto &material = model.materials[materialIndex];
      const auto &pbrMetallicRoughness = material.pbrMetallicRoughness;

      if (uBaseColorTexture >= 0) {
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
        glUniform1i(uBaseColorTexture, 0);
      }

      if (uBaseColorFactor >= 0) {
        glUniform4f(uBaseColorFactor,
            (float)pbrMetallicRoughness.baseColorFactor[0],
            (float)pbrMetallicRoughness.baseColorFactor[1],
            (float)pbrMetallicRoughness.baseColorFactor[2],
            (float)pbrMetallicRoughness.baseColorFactor[3]);
      }

      if (uMetallicFactor >= 0) {
        glUniform1f(
            uMetallicFactor, (float)pbrMetallicRoughness.metallicFactor);
      }
      if (uRoughnessFactor >= 0) {
        glUniform1f(
            uRoughnessFactor, (float)pbrMetallicRoughness.roughnessFactor);
      }
      if (uMetallicRoughnessTexture > 0) {
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
        glUniform1i(uMetallicRoughnessTexture, 1);
      }

      if (uEmissiveTexture >= 0) {

        auto textureObject = textureObjects[material.emissiveTexture.index];
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textureObject);
        glUniform1i(uEmissiveTexture, 2);
      }
      if (uEmissiveFactor >= 0) {
        auto emissiveFactor = material.emissiveFactor;
        glUniform3f(uEmissiveFactor, (float)emissiveFactor[0],
            (float)emissiveFactor[1], (float)emissiveFactor[2]);
      }
    } else {
      if (uBaseColorTexture >= 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(uBaseColorTexture, 0);
      }
      if (uBaseColorFactor >= 0) {
        glUniform4f(uBaseColorFactor, 1, 1, 1, 1);
      }
      if (uMetallicFactor >= 0) {
        glUniform1f(uMetallicFactor, 0.f);
      }
      if (uRoughnessFactor >= 0) {
        glUniform1f(uRoughnessFactor, 0.f);
      }
      if (uMetallicRoughnessTexture >= 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(uMetallicRoughnessTexture, 0);
      }
      if (uEmissiveTexture >= 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(uMetallicRoughnessTexture, 0);
      }
      if (uMetallicFactor >= 0) {
        glUniform3f(uMetallicRoughnessTexture, 0, 0, 0);
      }
    }
  };

  /** Lambda function to draw the scene **/
  const auto drawScene = [&](const Camera &camera) {
    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto viewMatrix = camera.getViewMatrix();

    /** Directionnal Light **/
    if (uLightDirection >= 0) {
      if (lightFromCamera)
        glUniform3f(uLightDirection, 0, 0, 1);
      else
        glUniform3fv(uLightDirection, 1,
            glm::value_ptr(glm::normalize(
                glm::vec3(viewMatrix * glm::vec4(lightDirection, 0.)))));
    }
    if (uLightIntensity >= 0) {
      glUniform3fv(uLightIntensity, 1, glm::value_ptr(lightIntensity));
    }

    /** Point Light **/
    if (uPointLightPosition >= 0) {
      if (enablePointLight) {
        glUniform3fv(uPointLightPosition, 1,
            glm::value_ptr(glm::normalize(
                glm::vec3(viewMatrix * glm::vec4(pointLightPosition, 1.)))));
        glUniform3fv(uPointLightColor, 1, glm::value_ptr(pointLightIntensity));
        glUniform1f(uPointLightConstant, 1.0f);
        glUniform1f(uPointLightLinear, 0.09f);
        glUniform1f(uPointLightQuadratic, 0.032f);
      } else {
        glUniform3fv(uPointLightColor, 1, glm::value_ptr(glm::vec3(0, 0, 0)));
      }
    }

    /** Spot Light **/
    if (uSpotLightPosition >= 0) {
      if (enableSpotLight) {
        glUniform3fv(uSpotLightPosition, 1, glm::value_ptr(spotLightPosition));
        glUniform3fv(
            uSpotLightDirection, 1, glm::value_ptr(spotLightDirection));
        glUniform3fv(uSpotLightColor, 1, glm::value_ptr(spotLightIntensity));
        glUniform1f(uSpotLightCutOff, glm::cos(glm::radians(spotLightCutOff)));
        glUniform1f(uSpotLightOuterCutOff,
            glm::cos(glm::radians(spotLightOuterCutOff)));
        glUniform1f(uSpotLightConstant, 1.0f);
        glUniform1f(uSpotLightLinear, 0.09f);
        glUniform1f(uSpotLightQuadratic, 0.032f);
      } else {
        glUniform3fv(uSpotLightColor, 1, glm::value_ptr(glm::vec3(0.f)));
        glUniform1f(uSpotLightCutOff, glm::cos(glm::radians(0.f)));
      }
    }

    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          // TODO The drawNode function
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
                modelViewMatrix, 1, GL_FALSE, glm::value_ptr(MV));
            glUniformMatrix4fv(
                modelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(MVP));
            glUniformMatrix4fv(normalMatrix, 1, GL_FALSE, glm::value_ptr(N));

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
        /** Parameter of Directionnal Light **/
        if (ImGui::CollapsingHeader(
                "Direcional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
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

          ImGui::Text("Directional light -> %.3f %.3f %.3f", lightDirection.x,
              lightDirection.y, lightDirection.z);

          static auto lightColor = glm::vec3(1.f);
          static float lightIntensityFactor = 1.f;

          if (ImGui::SliderFloat(
                  "dirIntensity", &lightIntensityFactor, 0, 10.f) ||
              ImGui::ColorEdit3(
                  "dirColor", reinterpret_cast<float *>(&lightColor))) {
            lightIntensity = lightColor * lightIntensityFactor;
          }
        }
        ImGui::Checkbox("Light from camera", &lightFromCamera);

        /** Parameter of Point Light **/
        if (ImGui::CollapsingHeader(
                "Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
          static auto pointLightColor = glm::vec3(1.f);
          static float lightIntensityFactor = 1.f;
          if (ImGui::SliderFloat("posX", &pointLightPosition.x, -10, 10.f) ||
              ImGui::SliderFloat("posY", &pointLightPosition.y, -10, 10.f) ||
              ImGui::SliderFloat("posZ", &pointLightPosition.z, -10, 10.f)) {
            ImGui::Text("Point light -> %.3f %.3f %.3f", pointLightPosition.x,
                pointLightPosition.y, pointLightPosition.z);
          }
          if (ImGui::SliderFloat(
                  "pointIntensity", &lightIntensityFactor, 0, 10.f) ||
              ImGui::ColorEdit3(
                  "pointColor", reinterpret_cast<float *>(&pointLightColor))) {
            pointLightIntensity = pointLightColor * lightIntensityFactor;
          }
        }
        ImGui::Checkbox("enable/Disable Point Light ", &enablePointLight);

        /** Parameter of Spot Light **/
        if (ImGui::CollapsingHeader(
                "Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
          static auto spotLightColor = glm::vec3(1.f);
          static float lightIntensityFactor = 1.f;

          if (ImGui::SliderFloat("posSLX", &spotLightPosition.x, -10, 10.f) ||
              ImGui::SliderFloat("posSLY", &spotLightPosition.y, -10, 10.f) ||
              ImGui::SliderFloat("posSLZ", &spotLightPosition.z, -10, 10.f) ||
              ImGui::SliderFloat(
                  "CutOff", &spotLightCutOff, 0.f, spotLightOuterCutOff) ||
              ImGui::SliderFloat("OuterCutOff", &spotLightOuterCutOff,
                  spotLightCutOff, 20.f)) {
            ImGui::Text("Spot light Position -> %.3f %.3f %.3f",
                spotLightPosition.x, spotLightPosition.y, spotLightPosition.z);
          }
          if (ImGui::SliderFloat(
                  "spotIntensity", &lightIntensityFactor, 0, 10.f) ||
              ImGui::ColorEdit3(
                  "spotColor", reinterpret_cast<float *>(&spotLightColor))) {
            spotLightIntensity = spotLightColor * lightIntensityFactor;
          }
        }
        ImGui::Checkbox("enable/Disable Spot Light", &enableSpotLight);
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
    const tinygltf::Model &model)
{
  // create a vector of GLuint with the correct size (model.buffers.size()) and
  // use glGenBuffers to create buffer objects.
  std::vector<GLuint> bufferObjects(model.buffers.size(), 0);
  glGenBuffers(GLsizei(model.buffers.size()), bufferObjects.data());
  for (size_t i = 0; i < model.buffers.size(); ++i) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
    // glBufferStorage in 4.4, etant limité sur 4.3 je doit utiliser
    // glBufferData
    glBufferData(GL_ARRAY_BUFFER, model.buffers[i].data.size(),
        model.buffers[i].data.data(), GL_STATIC_DRAW);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return bufferObjects;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects(
    const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects,
    std::vector<VaoRange> &meshIndexToVaoRange)
{
  std::vector<GLuint> vertexArrayObjects;
  const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
  const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
  const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;

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

    /* Here Start */
    // vaoOffset represente le debut du poiteur sur le tableau et
    // numberOfPrimitive la taille
    glGenVertexArrays(numberOfPrimitives, &vertexArrayObjects[vaoOffset]);
    for (size_t pimitiveIndice = 0; pimitiveIndice < size_t(numberOfPrimitives);
         ++pimitiveIndice) {
      const auto vao = vertexArrayObjects[vaoOffset + pimitiveIndice];
      const auto &primitive = mesh.primitives[pimitiveIndice];
      glBindVertexArray(vao);
      { // I'm opening a scope because I want to reuse the variable iterator in
        // the code for NORMAL and TEXCOORD_0
        // const std::string parameters[] = {"POSITION", "NORMAL",
        // "TEXCOORD_0"};
        const GLuint parametersVertexAttribs[] = {VERTEX_ATTRIB_POSITION_IDX,
            VERTEX_ATTRIB_NORMAL_IDX, VERTEX_ATTRIB_TEXCOORD0_IDX};
        std::map<int, std::string> mymap;
        mymap[VERTEX_ATTRIB_POSITION_IDX] = "POSITION";
        mymap[VERTEX_ATTRIB_NORMAL_IDX] = "NORMAL";
        mymap[VERTEX_ATTRIB_TEXCOORD0_IDX] = "TEXCOORD_0";

        for (const GLuint vertexAttrib : parametersVertexAttribs) {
          // std::cout <<  "Nous traitons l'attribut " <<
          // mymap.find(vertexAttrib)->second << std::endl;
          const auto iterator =
              primitive.attributes.find(mymap.find(vertexAttrib)->second);

          if (iterator != end(primitive.attributes)) { // If "POSITION" has been
                                                       // found in the map yep
            // (*iterator).first is the key "POSITION", (*iterator).second is
            // the value, ie. the index of the accessor for this attribute
            const auto accessorIdx = (*iterator).second;
            const auto &accessor =
                model.accessors[accessorIdx]; // TODO get the correct
                                              // tinygltf::Accessor from
                                              // model.accessors
            const auto &bufferView =
                model.bufferViews[accessor
                                      .bufferView]; // TODO get the correct
                                                    // tinygltf::BufferView from
                                                    // model.bufferViews. You
                                                    // need to use the accessor
            const auto bufferIdx =
                bufferView.buffer; // TODO get the index of the buffer used by
                                   // the bufferView (you need to use it)

            const auto bufferObject =
                bufferObjects[bufferIdx]; // TODO get the correct buffer object
                                          // from the buffer index

            // TODO Enable the vertex attrib array corresponding to POSITION
            // with glEnableVertexAttribArray (you need to use
            // VERTEX_ATTRIB_POSITION_IDX which has to be defined at the top of
            // the cpp file) Correction de l'attribut
            glEnableVertexAttribArray(vertexAttrib);
            assert(GL_ARRAY_BUFFER == bufferView.target);
            // TODO Bind the buffer object to GL_ARRAY_BUFFER
            glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

            const auto byteOffset =
                accessor.byteOffset +
                bufferView.byteOffset; // TODO Compute the total byte offset
                                       // using the accessor and the buffer view
            // TODO Call glVertexAttribPointer with the correct arguments.
            glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
            // Correction de l'attribut
            glVertexAttribPointer(vertexAttrib, accessor.type,
                accessor.componentType, GL_FALSE,
                GLsizei(bufferView.byteStride), (const GLvoid *)byteOffset);
          }
        }

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
      }
    }
    /* Here Finish*/
    compteur++;
  }
  glBindVertexArray(0);
  return vertexArrayObjects;
}

std::vector<GLuint> ViewerApplication::createTextureObjects(
    const tinygltf::Model &model) const
{
  std::vector<GLuint> textureObjects(model.textures.size(), 0);

  /** Definition Default Simpler dans le cas ou ils ne sont pas definit dans le
   * model **/
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