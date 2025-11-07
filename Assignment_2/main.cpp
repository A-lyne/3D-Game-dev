#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <map>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "common.hpp"

// Window dimensions
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Earth parameters
float animationTime = 0.0f;
float rotationSpeed = 0.3f;
float earthScale = 1.0f;
bool isRotating = true;
bool showWireframe = false;

// Earth lighting parameters
float lightIntensity = 1.0f;
glm::vec3 sunPosition = glm::vec3(5.0f, 3.0f, 5.0f);
glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);

// Earth structure
struct EarthModel {
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int diffuseTexture;
    unsigned int cloudsTexture;
    unsigned int nightLightsTexture;
    bool loaded;
};

EarthModel earth;

// GLTF Model structure
struct GLTFModel {
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
    bool loaded;
};

GLTFModel parametricPattern;

// Function to load GLTF model (simplified version)
bool loadGLTFModel(const std::string& gltfPath, GLTFModel& model)
{
    std::ifstream file(gltfPath);
    if (!file.is_open()) {
        std::cout << "Failed to open GLTF file: " << gltfPath << std::endl;
        return false;
    }
    
    std::string jsonContent((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();
    
    // Simple JSON parsing for GLTF (this is a basic implementation)
    // In a real project, you'd use a proper JSON library like nlohmann/json
    
    // For now, we'll create a simple parametric pattern manually
    // This creates a geometric pattern similar to the GLTF model
    
    model.vertices.clear();
    model.indices.clear();
    
    // Create a parametric pattern with lines
    int segments = 50;
    float width = 2.0f;
    float height = 1.0f;
    
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            float x = (float)i / segments * width - width/2.0f;
            float y = (float)j / segments * height - height/2.0f;
            float z = 0.0f;
            
            // Create parametric pattern
            float pattern = sin(x * 3.14159f) * cos(y * 3.14159f) * 0.1f;
            z += pattern;
            
            // Add vertex
            model.vertices.push_back(x);
            model.vertices.push_back(y);
            model.vertices.push_back(z);
            
            // Add bright cyan color for pattern visibility
            model.vertices.push_back(0.0f);
            model.vertices.push_back(1.0f);
            model.vertices.push_back(1.0f);
        }
    }
    
    // Create indices for lines
    for (int i = 0; i < segments - 1; ++i) {
        for (int j = 0; j < segments - 1; ++j) {
            int current = i * segments + j;
            int right = current + 1;
            int down = (i + 1) * segments + j;
            int diagonal = down + 1;
            
            // Create line pattern
            if ((i + j) % 2 == 0) {
                model.indices.push_back(current);
                model.indices.push_back(diagonal);
            } else {
                model.indices.push_back(right);
                model.indices.push_back(down);
            }
        }
    }
    
    // Set bounds
    model.minBounds = glm::vec3(-width/2.0f, -height/2.0f, -0.1f);
    model.maxBounds = glm::vec3(width/2.0f, height/2.0f, 0.1f);
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);
    
    glBindVertexArray(model.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(float), 
                 model.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), 
                 model.indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    model.loaded = true;
    std::cout << "Parametric pattern model loaded successfully with " << model.vertices.size()/6 
              << " vertices and " << model.indices.size() << " indices" << std::endl;
    
    return true;
}

// Function to load shader from file
std::string loadShaderFromFile(const std::string& filePath)
{
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try
    {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath << std::endl;
        return "";
    }
}

// Forward declarations
bool loadEarthModel(const std::string& objPath, EarthModel& model);
void createFallbackEarth();
unsigned int loadTexture(const char* path);
void processFaceVertex(const std::string& vertex, std::vector<unsigned int>& posIndices, 
                      std::vector<unsigned int>& texIndices, std::vector<unsigned int>& normIndices);

// Initialize earth model
void initializeEarth()
{
    // Load earth model
    if (!loadEarthModel("resources/23-earth_photorealistic_2k/Earth 2K.obj", earth)) {
        std::cout << "Failed to load earth model, creating fallback sphere..." << std::endl;
        // Create a simple sphere as fallback
        createFallbackEarth();
    }
    
    // Load textures
    std::cout << "Loading textures..." << std::endl;
    earth.diffuseTexture = loadTexture("resources/23-earth_photorealistic_2k/Textures/Diffuse_2K.png");
    earth.cloudsTexture = loadTexture("resources/23-earth_photorealistic_2k/Textures/Clouds_2K.png");
    earth.nightLightsTexture = loadTexture("resources/23-earth_photorealistic_2k/Textures/Night_lights_2K.png");
    
    // Check if textures loaded successfully
    std::cout << "Diffuse texture ID: " << earth.diffuseTexture << std::endl;
    std::cout << "Clouds texture ID: " << earth.cloudsTexture << std::endl;
    std::cout << "Night lights texture ID: " << earth.nightLightsTexture << std::endl;
    
    if (earth.diffuseTexture == 0 || earth.cloudsTexture == 0 || earth.nightLightsTexture == 0) {
        std::cout << "ERROR: Some textures failed to load!" << std::endl;
    } else {
        std::cout << "All textures loaded successfully!" << std::endl;
    }
}

// Create a proper sphere for Earth
void createFallbackEarth()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    int segments = 64;  // More segments for better quality
    int rings = 32;     // More rings for better quality
    
    for (int i = 0; i <= rings; ++i) {
        float phi = M_PI * i / rings;
        for (int j = 0; j <= segments; ++j) {
            float theta = 2.0f * M_PI * j / segments;
            
            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);
            
            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // UV coordinates for equirectangular mapping
            // Convert spherical to UV coordinates
            float u = 0.5f + atan2(z, x) / (2.0f * M_PI);
            float v = 0.5f - asin(y) / M_PI;
            
            // Ensure UV coordinates are in [0, 1] range
            u = fmod(u + 1.0f, 1.0f);
            v = glm::clamp(v, 0.0f, 1.0f);
            
            vertices.push_back(u);
            vertices.push_back(v);
            
            // Normal
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            int current = i * (segments + 1) + j;
            int next = current + segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &earth.VAO);
    glGenBuffers(1, &earth.VBO);
    glGenBuffers(1, &earth.EBO);
    
    glBindVertexArray(earth.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, earth.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, earth.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    earth.vertices = vertices;
    earth.indices = indices;
    earth.loaded = true;
}

// Process input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    
    // Earth controls
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        isRotating = !isRotating;
    }
    
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        showWireframe = !showWireframe;
    }
    
    // Adjust light intensity
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        lightIntensity = glm::clamp(lightIntensity + deltaTime, 0.1f, 3.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        lightIntensity = glm::clamp(lightIntensity - deltaTime, 0.1f, 3.0f);
    }
    
    // Adjust earth size
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        earthScale = glm::clamp(earthScale + deltaTime, 0.5f, 3.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        earthScale = glm::clamp(earthScale - deltaTime, 0.5f, 3.0f);
    }
    
    // Adjust sun position
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        sunPosition.x += deltaTime * 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        sunPosition.x -= deltaTime * 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        sunPosition.y += deltaTime * 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        sunPosition.y -= deltaTime * 2.0f;
    }
}

// Mouse callback
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;
    static bool firstMouse = true;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    static float yaw = -90.0f;
    static float pitch = 0.0f;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Scroll callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    const float fov = 45.0f;
    static float currentFov = fov;
    currentFov -= (float)yoffset;
    if (currentFov < 1.0f)
        currentFov = 1.0f;
    if (currentFov > 45.0f)
        currentFov = 45.0f;
}

// Function to load texture
unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Load image using stb_image
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
            
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Texture loaded successfully: " << path << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    
    return textureID;
}

// Helper function to process face vertex
void processFaceVertex(const std::string& vertex, std::vector<unsigned int>& posIndices, 
                      std::vector<unsigned int>& texIndices, std::vector<unsigned int>& normIndices)
{
    std::string pos, tex, norm;
    
    // Parse vertex indices
    size_t slash1 = vertex.find('/');
    size_t slash2 = vertex.find('/', slash1 + 1);
    
    if (slash1 != std::string::npos) {
        pos = vertex.substr(0, slash1);
        if (slash2 != std::string::npos) {
            tex = vertex.substr(slash1 + 1, slash2 - slash1 - 1);
            norm = vertex.substr(slash2 + 1);
        } else {
            tex = vertex.substr(slash1 + 1);
        }
    } else {
        pos = vertex;
    }
    
    posIndices.push_back(std::stoi(pos) - 1);
    
    if (!tex.empty()) {
        texIndices.push_back(std::stoi(tex) - 1);
    }
    
    if (!norm.empty()) {
        normIndices.push_back(std::stoi(norm) - 1);
    }
}

// Function to load OBJ model
bool loadEarthModel(const std::string& objPath, EarthModel& model)
{
    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << objPath << std::endl;
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> posIndices, texIndices, normIndices;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vt") {
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            texCoords.push_back(tex);
        }
        else if (type == "vn") {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (type == "f") {
            std::vector<std::string> vertices;
            std::string vertex;
            while (iss >> vertex) {
                vertices.push_back(vertex);
            }
            
            // Handle both triangles (3 vertices) and quads (4 vertices)
            if (vertices.size() >= 3) {
                // Process first triangle
                processFaceVertex(vertices[0], posIndices, texIndices, normIndices);
                processFaceVertex(vertices[1], posIndices, texIndices, normIndices);
                processFaceVertex(vertices[2], posIndices, texIndices, normIndices);
                
                // If quad, add second triangle
                if (vertices.size() == 4) {
                    processFaceVertex(vertices[0], posIndices, texIndices, normIndices);
                    processFaceVertex(vertices[2], posIndices, texIndices, normIndices);
                    processFaceVertex(vertices[3], posIndices, texIndices, normIndices);
                }
            }
        }
    }
    file.close();
    
    // Debug output
    std::cout << "OBJ loaded: " << positions.size() << " positions, " 
              << texCoords.size() << " texture coords, " 
              << normals.size() << " normals" << std::endl;
    std::cout << "Indices: " << posIndices.size() << " pos, " 
              << texIndices.size() << " tex, " 
              << normIndices.size() << " norm" << std::endl;
    
    // Create vertices array
    model.vertices.clear();
    model.indices.clear();
    
    for (size_t i = 0; i < posIndices.size(); i++) {
        // Position
        glm::vec3 pos = positions[posIndices[i]];
        model.vertices.push_back(pos.x);
        model.vertices.push_back(pos.y);
        model.vertices.push_back(pos.z);
        
        // Texture coordinates
        if (i < texIndices.size() && texIndices[i] < texCoords.size()) {
            glm::vec2 tex = texCoords[texIndices[i]];
            model.vertices.push_back(tex.x);
            model.vertices.push_back(tex.y);
        } else {
            // Fallback UV coordinates if not available in OBJ
            glm::vec3 unit = glm::normalize(pos);
            float u = 0.5f + atan2(unit.z, unit.x) / (2.0f * M_PI);
            float v = 0.5f - asin(unit.y) / M_PI;
            u = fmod(u + 1.0f, 1.0f);
            v = glm::clamp(v, 0.0f, 1.0f);
            model.vertices.push_back(u);
            model.vertices.push_back(v);
        }
        
        // Normal
        if (i < normIndices.size() && normIndices[i] < normals.size()) {
            glm::vec3 norm = normals[normIndices[i]];
            model.vertices.push_back(norm.x);
            model.vertices.push_back(norm.y);
            model.vertices.push_back(norm.z);
        } else {
            model.vertices.push_back(0.0f);
            model.vertices.push_back(0.0f);
            model.vertices.push_back(1.0f);
        }
        
        model.indices.push_back(i);
    }
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);
    
    glBindVertexArray(model.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(float), 
                 model.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), 
                 model.indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    model.loaded = true;
    std::cout << "Earth model loaded successfully with " << model.vertices.size()/8 
              << " vertices and " << model.indices.size() << " indices" << std::endl;
    
    return true;
}

int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 2: Earth 3D Model", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        int error;
        const char* description;
        error = glfwGetError(&description);
        if (error != GLFW_NO_ERROR) {
            std::cout << "GLFW Error: " << description << std::endl;
        }
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize earth model
    initializeEarth();

    // Load and compile shader program
    std::string vertexShaderSource = loadShaderFromFile("resources/vs/kinetic_sculpture.vs");
    std::string fragmentShaderSource = loadShaderFromFile("resources/fs/kinetic_sculpture.fs");
    
    if (vertexShaderSource.empty() || fragmentShaderSource.empty())
    {
        std::cout << "Failed to load shader files" << std::endl;
        return -1;
    }
    
    const char* vertexShaderCode = vertexShaderSource.c_str();
    const char* fragmentShaderCode = fragmentShaderSource.c_str();
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Enable texture loading
    stbi_set_flip_vertically_on_load(true);

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update animation time
        animationTime = currentFrame;

        // Input
        processInput(window);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        glUseProgram(shaderProgram);

        // Pass projection matrix to shader
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Camera/view transformation
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
        // Lighting setup for earth
        glm::vec3 lightColor = sunColor * lightIntensity;
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(sunPosition));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        // Set wireframe mode if enabled
        if (showWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Render earth
        if (earth.loaded) {
            // Bind textures first
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, earth.diffuseTexture);
            glUniform1i(glGetUniformLocation(shaderProgram, "diffuseTex"), 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, earth.cloudsTexture);
            glUniform1i(glGetUniformLocation(shaderProgram, "cloudsTex"), 1);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, earth.nightLightsTexture);
            glUniform1i(glGetUniformLocation(shaderProgram, "nightTex"), 2);
            
            // Create model matrix for earth
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            
            // Apply rotation if enabled
            if (isRotating) {
                model = glm::rotate(model, animationTime * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            
            // Apply scaling
            model = glm::scale(model, glm::vec3(earthScale, earthScale, earthScale));
            
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // Bind VAO and draw
            glBindVertexArray(earth.VAO);
            glDrawElements(GL_TRIANGLES, earth.indices.size(), GL_UNSIGNED_INT, 0);
        }

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    if (earth.loaded) {
        glDeleteVertexArrays(1, &earth.VAO);
        glDeleteBuffers(1, &earth.VBO);
        glDeleteBuffers(1, &earth.EBO);
        glDeleteTextures(1, &earth.diffuseTexture);
        glDeleteTextures(1, &earth.cloudsTexture);
        glDeleteTextures(1, &earth.nightLightsTexture);
    }
    glDeleteProgram(shaderProgram);
    
    // Reset polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glfwTerminate();
    return 0;
}
