#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/cameras.hpp"

#include <stb_image_write.h>
#include <tiny_gltf.h>

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

  const auto modelViewProjMatrixLocation =glGetUniformLocation(glslProgram.glId(), "uModelViewProjMatrix");
  const auto modelViewMatrixLocation     =glGetUniformLocation(glslProgram.glId(), "uModelViewMatrix");
  const auto normalMatrixLocation        =glGetUniformLocation(glslProgram.glId(), "uNormalMatrix");

  // Build projection matrix
  auto maxDistance = 500.f; // TODO use scene bounds instead to compute this
  maxDistance = maxDistance > 0.f ? maxDistance : 100.f;
  const auto projMatrix =
      glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight,
          0.001f * maxDistance, 1.5f * maxDistance);

  // TODO Implement a new CameraController model and use it instead. Propose the
  // choice from the GUI
  FirstPersonCameraController cameraController{m_GLFWHandle.window(), 0.5f * maxDistance};
  if (m_hasUserCamera) {
    cameraController.setCamera(m_userCamera);
  } 
  else {
    // TODO Use scene bounds to compute a better default camera
    cameraController.setCamera(
        Camera{glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)});
  }

  tinygltf::Model model;
  // TODO Loading the glTF file
  if (!loadGltfFile(model)){
    std::cout << " Can't Load GltfFile !!!" << loadGltfFile(model) << std::endl;
    return false;
  }

  // TODO Creation of Buffer Objects
  const auto bufferObjects = createBufferObjects(model);

  // TODO Creation of Vertex Array Objects
  std::vector<VaoRange> meshToVertexArrays;
  const auto vertexArrayObjects = createVertexArrayObjects(model, bufferObjects, meshToVertexArrays);

  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);
  glslProgram.use();

  // Lambda function to draw the scene
  const auto drawScene = [&](const Camera &camera) {
    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto viewMatrix = camera.getViewMatrix();

    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode = [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          // TODO The drawNode function
          const tinygltf::Node &node = model.nodes[nodeIdx];
          #include "gltf.hpp"
          const glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);
        };

    // Draw the scene referenced by gltf file
    if (model.defaultScene >= 0) {
      // TODO Draw all nodes
      for (const auto nodeIndice : model.scenes[model.defaultScene].nodes){
         drawNode(nodeIndice, glm::mat4(1));
      }
    }
  };
  // Loop until the user closes the window
  for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose();
       ++iterationCount) {
    const auto seconds = glfwGetTime();

    const auto camera = cameraController.getCamera();
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
      }
      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    auto ellapsedTime = glfwGetTime() - seconds;
    auto guiHasFocus =
        ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    if (!guiHasFocus) {
      cameraController.update(float(ellapsedTime));
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

bool ViewerApplication::loadGltfFile(tinygltf::Model & model){
  std::cout << "Loading glTF File ..." << std::endl;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  bool ret =  loader.LoadASCIIFromFile(&model, &err, &warn, m_gltfFilePath.string());

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


std::vector<GLuint> ViewerApplication::createBufferObjects( const tinygltf::Model &model){
  //create a vector of GLuint with the correct size (model.buffers.size()) and use glGenBuffers to create buffer objects.
  std::vector<GLuint> bufferObjects(model.buffers.size(), 0);
  glGenBuffers(GLsizei(model.buffers.size()), bufferObjects.data());
  for (size_t i = 0; i < model.buffers.size(); ++i) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
    // glBufferStorage in 4.4, etant limité sur 4.3 je doit utiliser glBufferData
    glBufferData(GL_ARRAY_BUFFER, model.buffers[i].data.size(),model.buffers[i].data.data(), GL_STATIC_DRAW);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return bufferObjects;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects( const tinygltf::Model &model, const std::vector<GLuint> &bufferObjects, std::vector<VaoRange> & meshIndexToVaoRange){
  std::vector<GLuint> vertexArrayObjects;
  const GLuint VERTEX_ATTRIB_POSITION_IDX = 0;
  const GLuint VERTEX_ATTRIB_NORMAL_IDX = 1;
  const GLuint VERTEX_ATTRIB_TEXCOORD0_IDX = 2;

  // { Indice of Mesh , Number of primitives}  need this to Draw After
  meshIndexToVaoRange.resize(model.meshes.size());
  // const auto vaoOffset = vertexArrayObjects.size();
  // vertexArrayObjects.resize(vaoOffset + model.meshes[meshIdx].primitives.size());
  // meshIndexToVaoRange.push_back(VaoRange{vaoOffset, model.meshes[meshIdx].primitives.size()});

  int compteur = 0;
  for (const auto &mesh : model.meshes){
    auto vaoOffset = vertexArrayObjects.size();
    meshIndexToVaoRange[compteur].begin = vaoOffset;
    auto numberOfPrimitives  = mesh.primitives.size();
    meshIndexToVaoRange[compteur].count = numberOfPrimitives;

    // resize vector of VAOs en ajoutant a chque iteration le nombre de primitives de la "Mesh"
    vertexArrayObjects.resize(vaoOffset + mesh.primitives.size());

    /* Here Start */
    // vaoOffset represente le debut du poiteur sur le tableau et numberOfPrimitive la taille
    glGenVertexArrays (numberOfPrimitives,&vertexArrayObjects[vaoOffset]);
    for (size_t pimitiveIndice = 0; pimitiveIndice < numberOfPrimitives ; ++pimitiveIndice) {
      const auto vao = vertexArrayObjects[vaoOffset + pimitiveIndice];
      const auto &primitive = mesh.primitives[pimitiveIndice];
      glBindVertexArray(vao);
      { // I'm opening a scope because I want to reuse the variable iterator in the code for NORMAL and TEXCOORD_0
        // const std::string parameters[] = {"POSITION", "NORMAL", "TEXCOORD_0"};
        const GLuint parametersVertexAttribs[] = {VERTEX_ATTRIB_POSITION_IDX, VERTEX_ATTRIB_NORMAL_IDX, VERTEX_ATTRIB_TEXCOORD0_IDX};
        std::map<int,std::string> mymap;
            mymap[VERTEX_ATTRIB_POSITION_IDX] = "POSITION";
            mymap[VERTEX_ATTRIB_NORMAL_IDX]   = "NORMAL";
            mymap[VERTEX_ATTRIB_TEXCOORD0_IDX]= "TEXCOORD_0";

        for(const auto vertexAttrib : parametersVertexAttribs ){
          std::cout <<  "Nous traitons l'attribut " << mymap.find(vertexAttrib)->second << std::endl;
          const auto iterator = primitive.attributes.find(mymap.find(vertexAttrib)->second);

          if (iterator != end(primitive.attributes)) { // If "POSITION" has been found in the map yep
            // (*iterator).first is the key "POSITION", (*iterator).second is the value, ie. the index of the accessor for this attribute
            const auto accessorIdx = (*iterator).second;
            const auto &accessor = model.accessors[accessorIdx];              // TODO get the correct tinygltf::Accessor from model.accessors
            const auto &bufferView = model.bufferViews[accessor.bufferView];  // TODO get the correct tinygltf::BufferView from model.bufferViews. You need to use the accessor
            const auto bufferIdx = bufferView.buffer;                         // TODO get the index of the buffer used by the bufferView (you need to use it)

            const auto bufferObject = bufferObjects[bufferIdx];// TODO get the correct buffer object from the buffer index

            // TODO Enable the vertex attrib array corresponding to POSITION with glEnableVertexAttribArray (you need to use VERTEX_ATTRIB_POSITION_IDX which has to be defined at the top of the cpp file)
            glEnableVertexAttribArray(VERTEX_ATTRIB_POSITION_IDX);
            // TODO Bind the buffer object to GL_ARRAY_BUFFER
            glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

            const auto byteOffset = accessor.byteOffset + bufferView.byteOffset;// TODO Compute the total byte offset using the accessor and the buffer view
            // TODO Call glVertexAttribPointer with the correct arguments. 
            glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]);
            glVertexAttribPointer(VERTEX_ATTRIB_NORMAL_IDX, accessor.type,accessor.componentType, GL_FALSE, GLsizei(bufferView.byteStride),
                                  (const GLvoid *)byteOffset);
          }
        }

        // Ajout aprés oublie    
        if ( primitive.indices >= 0){
          const auto accessorIdx = primitive.indices;
            const auto &accessor = model.accessors[accessorIdx];
            const auto &bufferView = model.bufferViews[accessor.bufferView];
            const auto bufferIdx = bufferView.buffer;
            const auto bufferObject = bufferObjects[bufferIdx];
            assert(GL_ARRAY_BUFFER == bufferView.target);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bufferObject);
        }
      }
    }
    /* Here Finish*/
    compteur++;
  }
  glBindVertexArray(0);
  return vertexArrayObjects;
}