#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#ifdef __APPLE__
#include <unistd.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#elif defined(_WIN32)
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#define PATH_MAX _MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "camera.h"
#include "model.h"
#include "../common/include/common.hpp"

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 5.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Player
glm::vec3 playerPosition(0.0f, 0.0f, 0.0f);
float playerSpeed = 5.0f;
float playerRotation = 0.0f;

// Game state
std::vector<glm::vec3> items; // Item positions
std::vector<bool> itemCollected; // Track collected items
const float GROUND_SIZE = 20.0f;
const float GROUND_Y = -0.5f;

// Collision detection
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB(glm::vec3 pos, glm::vec3 size) {
        min = pos - size * 0.5f;
        max = pos + size * 0.5f;
    }
};

bool checkCollision(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int createPlaneVAO();
unsigned int createCubeVAO();

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 3 - 3D Game", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Configure global opengl state
    glEnable(GL_DEPTH_TEST);
    
    // Print current working directory for debugging
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
    
    // Helper function to find resource path
    auto findResourcePath = [](const std::string& relativePath) -> std::string {
        // Try paths in order: Debug dir, build dir, source dir
        std::vector<std::string> pathsToTry = {
            "../" + relativePath,  // From Debug directory (build/Assignment_3/Debug/)
            relativePath,  // Current directory (if running from build/Assignment_3/)
            "../../Assignment_3/" + relativePath,  // From project root
            "../../../Assignment_3/" + relativePath  // From deeper in build tree
        };
        
        std::cout << "Searching for: " << relativePath << std::endl;
        for (const auto& path : pathsToTry) {
            std::ifstream test(path);
            if (test.good()) {
                test.close();
                std::cout << "  Found at: " << path << std::endl;
                return path;
            }
            test.close();
        }
        std::cout << "  WARNING: Not found in any location!" << std::endl;
        // Return original path if none found (will show error later)
        return relativePath;
    };
    
    // Build and compile shaders
    std::string vsPath = findResourcePath("resource/shaders/model.vs");
    std::string fsPath = findResourcePath("resource/shaders/model.fs");
    std::cout << "Loading vertex shader from: " << vsPath << std::endl;
    std::cout << "Loading fragment shader from: " << fsPath << std::endl;
    
    Common::Shader shader(vsPath.c_str(), fsPath.c_str());
    
    // Load player model
    std::string modelPath = findResourcePath("resource/Torque Twister/Torque Twister.obj");
    std::cout << "Loading model from: " << modelPath << std::endl;
    Model playerModel(modelPath.c_str());
    
    // Initialize items (boxes)
    items = {
        glm::vec3(5.0f, 0.5f, 5.0f),
        glm::vec3(-5.0f, 0.5f, 5.0f),
        glm::vec3(5.0f, 0.5f, -5.0f),
        glm::vec3(-5.0f, 0.5f, -5.0f),
        glm::vec3(0.0f, 0.5f, 8.0f),
        glm::vec3(8.0f, 0.5f, 0.0f),
    };
    itemCollected.resize(items.size(), false);
    
    // Create ground plane
    unsigned int planeVAO = createPlaneVAO();
    
    // Create cube VAO for items
    unsigned int cubeVAO = createCubeVAO();
    
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Input
        processInput(window);
        
        // Update camera to follow player
        camera.FollowTarget(playerPosition, 0.0f, 5.0f, 10.0f);
        
        // Check collisions with items
        AABB playerBox(playerPosition, glm::vec3(1.0f, 1.0f, 1.0f));
        for (size_t i = 0; i < items.size(); i++) {
            if (!itemCollected[i]) {
                AABB itemBox(items[i], glm::vec3(1.0f, 1.0f, 1.0f));
                if (checkCollision(playerBox, itemBox)) {
                    itemCollected[i] = true;
                    std::cout << "Item collected! " << (items.size() - std::count(itemCollected.begin(), itemCollected.end(), true)) << " items remaining." << std::endl;
                }
            }
        }
        
        // Check collision with ground boundaries
        if (playerPosition.x > GROUND_SIZE / 2.0f) playerPosition.x = GROUND_SIZE / 2.0f;
        if (playerPosition.x < -GROUND_SIZE / 2.0f) playerPosition.x = -GROUND_SIZE / 2.0f;
        if (playerPosition.z > GROUND_SIZE / 2.0f) playerPosition.z = GROUND_SIZE / 2.0f;
        if (playerPosition.z < -GROUND_SIZE / 2.0f) playerPosition.z = -GROUND_SIZE / 2.0f;
        
        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.use();
        
        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        
        // Lighting
        glm::vec3 lightPos(10.0f, 10.0f, 10.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("lightColor", lightColor);
        shader.setVec3("viewPos", camera.Position);
        
        // Render ground plane
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, GROUND_Y, 0.0f));
        model = glm::scale(model, glm::vec3(GROUND_SIZE, 1.0f, GROUND_SIZE));
        shader.setMat4("model", model);
        shader.setBool("useTexture", false);
        shader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f)); // Gray for ground
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Render player
        model = glm::mat4(1.0f);
        model = glm::translate(model, playerPosition);
        model = glm::rotate(model, glm::radians(playerRotation + 270.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate 270 degrees to fix orientation
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f)); // Scale the model (increased from 0.1 to 0.5)
        shader.setMat4("model", model);
        shader.setBool("useTexture", true);
        shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White fallback
        playerModel.Draw(shader.ID);
        
        // Render items (boxes) - with a slight rotation for visual appeal
        for (size_t i = 0; i < items.size(); i++) {
            if (!itemCollected[i]) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, items[i]);
                model = glm::rotate(model, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
                shader.setMat4("model", model);
                shader.setBool("useTexture", false);
                shader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 1.0f)); // Blue for items
                glBindVertexArray(cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        
        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    
    glfwTerminate();
    return 0;
}

// Process all input
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Player movement
    glm::vec3 moveDirection(0.0f);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveDirection += glm::vec3(0.0f, 0.0f, -1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveDirection += glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        moveDirection += glm::vec3(-1.0f, 0.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        moveDirection += glm::vec3(1.0f, 0.0f, 0.0f);
    }
    
    // Normalize movement direction
    if (glm::length(moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);
        playerPosition += moveDirection * playerSpeed * deltaTime;
        
        // Update player rotation to face movement direction
        playerRotation = glm::degrees(atan2(moveDirection.x, moveDirection.z));
    }
}

// glfw: whenever the window size changed
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// Create a plane VAO
unsigned int createPlaneVAO() {
    float vertices[] = {
        // positions          // normals         // texcoords
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };
    
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    
    glBindVertexArray(0);
    
    return VAO;
}

// Create a cube VAO
unsigned int createCubeVAO() {
    float vertices[] = {
        // positions          // normals         // texcoords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
    
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    
    glBindVertexArray(0);
    
    return VAO;
}
