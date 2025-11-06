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

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "common.hpp"

// Window dimensions
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Camera (mouse controlled)
glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 8.0f;
float cameraHeight = 3.0f;

// Mouse control
float yaw = 0.0f;     // Horizontal rotation (0° = facing +X, -180° = facing -X)
float pitch = 0.0f;   // Vertical rotation
float fov = 45.0f;    // Field of view (for zoom)
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Player rotation (in degrees, around Y axis)
float playerRotation = 0.0f;  // 0 = facing forward (negative Z), 90 = facing right, etc.
bool playerRotating = false;   // Track if player is rotating
bool isADMovement = false;     // Track if current movement is A or D key

// Object structure
struct GameObject {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    glm::vec3 minBounds;  // AABB minimum bounds
    glm::vec3 maxBounds;  // AABB maximum bounds
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int textureID;  // Texture ID (0 = no texture)
    bool loaded;
    bool hasTexture;
};

// Game objects
GameObject player;           // Player model
GameObject scene;            // Scene/map model (Hummer EV)
GameObject ground;           // Ground/floor plane
GameObject obstacle1, obstacle2, obstacle3;  // Obstacles for collision testing
std::vector<GameObject> collisionBoxes;  // Multiple boxes for collision testing
std::vector<GameObject> items;    // Collectible items
std::vector<GameObject> enemies;  // Enemy models

// Helper function to process face vertex indices from OBJ file
void processFaceVertex(const std::string& vertex, std::vector<unsigned int>& posIndices, 
                      std::vector<unsigned int>& texIndices, std::vector<unsigned int>& normIndices) {
    std::string pos, tex, norm;
    
    // Parse vertex indices (format: pos/tex/norm or pos//norm or pos)
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
    
    if (!pos.empty()) {
        posIndices.push_back(std::stoi(pos) - 1);  // OBJ uses 1-based indexing
    }
    if (!tex.empty()) {
        texIndices.push_back(std::stoi(tex) - 1);
    }
    if (!norm.empty()) {
        normIndices.push_back(std::stoi(norm) - 1);
    }
}

// Load texture from file
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    // Flip texture vertically (OpenGL expects bottom-left origin)
    stbi_set_flip_vertically_on_load(true);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        std::cout << "Texture loaded: " << path << " (" << width << "x" << height << ", " << nrComponents << " channels)" << std::endl;
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
    
    return textureID;
}

// Helper function to find model/texture file
std::string findModelFile(const std::string& filename) {
    std::vector<std::string> paths = {
        filename,
        "resources/" + filename,
        "../" + filename,
        "../resources/" + filename
    };
    
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.good()) {
            file.close();
            return path;
        }
    }
    
    return filename;
}

// Load OBJ model from file
bool loadOBJModel(const std::string& objPath, GameObject& obj) {
    std::string fullPath = findModelFile(objPath);
    std::ifstream file(fullPath);
    
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << fullPath << std::endl;
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
    
    // Calculate AABB bounds
    if (!positions.empty()) {
        obj.minBounds = positions[0];
        obj.maxBounds = positions[0];
        for (const auto& pos : positions) {
            obj.minBounds.x = std::min(obj.minBounds.x, pos.x);
            obj.minBounds.y = std::min(obj.minBounds.y, pos.y);
            obj.minBounds.z = std::min(obj.minBounds.z, pos.z);
            obj.maxBounds.x = std::max(obj.maxBounds.x, pos.x);
            obj.maxBounds.y = std::max(obj.maxBounds.y, pos.y);
            obj.maxBounds.z = std::max(obj.maxBounds.z, pos.z);
        }
    }
    
    // Create vertices array
    obj.vertices.clear();
    obj.indices.clear();
    
    for (size_t i = 0; i < posIndices.size(); i++) {
        // Position
        glm::vec3 pos = positions[posIndices[i]];
        obj.vertices.push_back(pos.x);
        obj.vertices.push_back(pos.y);
        obj.vertices.push_back(pos.z);
        
        // Texture coordinates
        if (i < texIndices.size() && texIndices[i] < texCoords.size()) {
            glm::vec2 tex = texCoords[texIndices[i]];
            obj.vertices.push_back(tex.x);
            obj.vertices.push_back(tex.y);
        } else {
            // Fallback UV coordinates
            obj.vertices.push_back(0.0f);
            obj.vertices.push_back(0.0f);
        }
        
        // Normal
        if (i < normIndices.size() && normIndices[i] < normals.size()) {
            glm::vec3 norm = normals[normIndices[i]];
            obj.vertices.push_back(norm.x);
            obj.vertices.push_back(norm.y);
            obj.vertices.push_back(norm.z);
        } else {
            // Calculate normal from position if not available
            glm::vec3 norm = glm::normalize(pos);
            obj.vertices.push_back(norm.x);
            obj.vertices.push_back(norm.y);
            obj.vertices.push_back(norm.z);
        }
        
        obj.indices.push_back(i);
    }
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);
    
    glBindVertexArray(obj.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER, obj.vertices.size() * sizeof(float), 
                 obj.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(unsigned int), 
                 obj.indices.data(), GL_STATIC_DRAW);
    
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
    
    obj.loaded = true;
    obj.hasTexture = false;
    obj.textureID = 0;
    
    std::cout << "OBJ model loaded successfully: " << fullPath << std::endl;
    std::cout << "  - Vertices: " << obj.vertices.size() / 8 << std::endl;
    std::cout << "  - Indices: " << obj.indices.size() << std::endl;
    
    return true;
}

// Simple cube model loader
void createCube(GameObject& obj, float size = 1.0f) {
    float halfSize = size / 2.0f;
    
    // Vertices: position (3) + texCoord (2) + normal (3) = 8 floats per vertex
    obj.vertices = {
        // Front face
        -halfSize, -halfSize,  halfSize,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
         halfSize, -halfSize,  halfSize,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
         halfSize,  halfSize,  halfSize,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
        -halfSize,  halfSize,  halfSize,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f,
        // Back face
        -halfSize, -halfSize, -halfSize,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         halfSize, -halfSize, -halfSize,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         halfSize,  halfSize, -halfSize,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f,
        -halfSize,  halfSize, -halfSize,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
        // Left face
        -halfSize, -halfSize, -halfSize,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
        -halfSize, -halfSize,  halfSize,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
        -halfSize,  halfSize,  halfSize,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -halfSize,  halfSize, -halfSize,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        // Right face
         halfSize, -halfSize, -halfSize,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
         halfSize,  halfSize, -halfSize,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
         halfSize,  halfSize,  halfSize,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
         halfSize, -halfSize,  halfSize,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
        // Bottom face
        -halfSize, -halfSize, -halfSize,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         halfSize, -halfSize, -halfSize,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         halfSize, -halfSize,  halfSize,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
        -halfSize, -halfSize,  halfSize,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
        // Top face
        -halfSize,  halfSize, -halfSize,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
        -halfSize,  halfSize,  halfSize,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f,
         halfSize,  halfSize,  halfSize,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
         halfSize,  halfSize, -halfSize,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f
    };
    
    // Indices
    obj.indices = {
        // Front
        0, 1, 2,  2, 3, 0,
        // Back
        4, 5, 6,  6, 7, 4,
        // Left
        8, 9, 10,  10, 11, 8,
        // Right
        12, 13, 14,  14, 15, 12,
        // Bottom
        16, 17, 18,  18, 19, 16,
        // Top
        20, 21, 22,  22, 23, 20
    };
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);
    
    glBindVertexArray(obj.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER, obj.vertices.size() * sizeof(float), obj.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(unsigned int), obj.indices.data(), GL_STATIC_DRAW);
    
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
    
    // Set AABB bounds (for intersection detection)
    obj.minBounds = glm::vec3(-halfSize, -halfSize, -halfSize);
    obj.maxBounds = glm::vec3(halfSize, halfSize, halfSize);
    
    obj.loaded = true;
}

// Create a large ground plane
void createGround(GameObject& obj, float size = 20.0f) {
    float halfSize = size / 2.0f;
    
    // Create a large flat plane
    obj.vertices = {
        // Position (3) + TexCoord (2) + Normal (3) = 8 floats per vertex
        -halfSize, 0.0f, -halfSize,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         halfSize, 0.0f, -halfSize,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         halfSize, 0.0f,  halfSize,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -halfSize, 0.0f,  halfSize,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };
    
    obj.indices = {
        0, 1, 2,
        2, 3, 0
    };
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);
    
    glBindVertexArray(obj.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER, obj.vertices.size() * sizeof(float), obj.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(unsigned int), obj.indices.data(), GL_STATIC_DRAW);
    
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
    
    // Set AABB bounds
    obj.minBounds = glm::vec3(-halfSize, -0.1f, -halfSize);
    obj.maxBounds = glm::vec3(halfSize, 0.1f, halfSize);
    
    obj.loaded = true;
}

// Initialize all objects
void initializeObjects() {
    std::cout << "\n=== Loading Models ===" << std::endl;
    
    // Create ground plane - make it much larger
    ground.position = glm::vec3(0.0f, 0.0f, 0.0f);
    ground.scale = glm::vec3(1.0f);
    ground.color = glm::vec3(0.3f, 0.3f, 0.3f);
    createGround(ground, 100.0f);  // Expanded from 40 to 100
    std::cout << "Ground plane created (100x100 units)" << std::endl;
    
    // Load Hummer EV model as PLAYER (not scene)
    // Start player at a safe position away from obstacles
    player.position = glm::vec3(0.0f, 0.0f, 8.0f);  // Move further back
    player.scale = glm::vec3(1.0f);  // Start with scale 1.0
    player.color = glm::vec3(0.8f, 0.8f, 0.8f);
    
    if (loadOBJModel("resources/hummer-ev-low-poly/source/HummerEV/Hummer_EV_2022_UV.obj", player)) {
        std::cout << "Hummer EV model loaded as PLAYER successfully!" << std::endl;
        
        // Print bounds to debug scale
        std::cout << "  Model bounds - Min: (" << player.minBounds.x << ", " << player.minBounds.y << ", " << player.minBounds.z << ")" << std::endl;
        std::cout << "  Model bounds - Max: (" << player.maxBounds.x << ", " << player.maxBounds.y << ", " << player.maxBounds.z << ")" << std::endl;
        
        // Calculate appropriate scale based on model size
        glm::vec3 modelSize = player.maxBounds - player.minBounds;
        float maxDim = std::max(modelSize.x, std::max(modelSize.y, modelSize.z));
        std::cout << "  Model size: (" << modelSize.x << ", " << modelSize.y << ", " << modelSize.z << ")" << std::endl;
        std::cout << "  Max dimension: " << maxDim << std::endl;
        
        // Scale to make model visible (target size around 5-10 units)
        if (maxDim > 0.1f) {
            float targetSize = 6.0f;
            player.scale = glm::vec3(targetSize / maxDim);
            std::cout << "  Adjusted scale to: " << player.scale.x << std::endl;
        }
        
        // Center the model vertically (adjust position based on bounds)
        // Make sure the bottom of the car is on the ground (y=0)
        glm::vec3 center = (player.minBounds + player.maxBounds) * 0.5f;
        float bottomY = player.minBounds.y * player.scale.y;
        player.position.y = -bottomY;  // Place bottom of car on ground
        std::cout << "  Player (Hummer) positioned at: (" << player.position.x << ", " << player.position.y << ", " << player.position.z << ")" << std::endl;
        
        // Try to load main body texture from multiple possible locations
        std::vector<std::string> texturePaths = {
            "resources/hummer-ev-low-poly/textures/Hummer_EV_2022_UV_Main_Body_BaseColor.png",
            "resources/hummer-ev-low-poly/source/HummerEV/Hummer_EV_2022_UV_Main_Body_BaseColor.png",
            "../resources/hummer-ev-low-poly/textures/Hummer_EV_2022_UV_Main_Body_BaseColor.png",
            "../resources/hummer-ev-low-poly/source/HummerEV/Hummer_EV_2022_UV_Main_Body_BaseColor.png"
        };
        
        bool textureLoaded = false;
        for (const auto& texPath : texturePaths) {
            std::string foundPath = findModelFile(texPath);
            if (foundPath.find("resources") != std::string::npos || foundPath.find("../") != std::string::npos) {
                player.textureID = loadTexture(foundPath.c_str());
                if (player.textureID != 0) {
                    player.hasTexture = true;
                    textureLoaded = true;
                    std::cout << "Hummer texture loaded from: " << foundPath << std::endl;
                    break;
                }
            }
        }
        
        if (!textureLoaded) {
            std::cout << "Warning: Could not load Hummer texture, using material color" << std::endl;
        }
    } else {
        std::cout << "Hummer EV model not found, using default cube for player" << std::endl;
        createCube(player, 1.0f);
    }
    
    // Scene is now just a static object (optional - can remove if not needed)
    scene.position = glm::vec3(0.0f, 0.0f, 0.0f);
    scene.scale = glm::vec3(1.0f);
    scene.color = glm::vec3(0.5f, 0.5f, 0.5f);
    scene.loaded = false;  // Don't render scene, only player
    
    // Create multiple collision boxes for testing
    std::cout << "Creating collision test boxes..." << std::endl;
    collisionBoxes.clear();
    
    // Create boxes in a pattern around the scene
    glm::vec3 boxColors[] = {
        glm::vec3(1.0f, 0.2f, 0.2f),  // Red
        glm::vec3(0.2f, 1.0f, 0.2f),  // Green
        glm::vec3(0.2f, 0.2f, 1.0f),  // Blue
        glm::vec3(1.0f, 1.0f, 0.2f),  // Yellow
        glm::vec3(1.0f, 0.2f, 1.0f),  // Magenta
        glm::vec3(0.2f, 1.0f, 1.0f),  // Cyan
    };
    
    // Create boxes in a grid pattern - make them smaller and spread out more
    int boxCount = 0;
    for (int x = -8; x <= 8; x += 3) {  // Wider spacing (3 instead of 2)
        for (int z = -8; z <= 8; z += 3) {
            // Skip center area and player start area (z around 8)
            if (x >= -3 && x <= 3 && z >= -3 && z <= 3) continue;
            if (z >= 5 && z <= 12) continue;  // Skip larger player start area
            
            GameObject box;
            box.position = glm::vec3(x * 3.0f, 0.25f, z * 3.0f);  // Further apart (3.0f spacing)
            box.scale = glm::vec3(1.0f);
            box.color = boxColors[boxCount % 6];
            createCube(box, 0.8f);  // Smaller boxes (0.8f instead of 1.5f)
            collisionBoxes.push_back(box);
            boxCount++;
        }
    }
    
    // Also create some obstacles - place them away from player start position (z=8)
    obstacle1.position = glm::vec3(6.0f, 0.4f, 0.0f);  // Further away
    obstacle1.scale = glm::vec3(1.0f);
    obstacle1.color = glm::vec3(1.0f, 0.5f, 0.0f);
    createCube(obstacle1, 1.2f);  // Smaller
    
    obstacle2.position = glm::vec3(-6.0f, 0.4f, 0.0f);  // Further away
    obstacle2.scale = glm::vec3(1.0f);
    obstacle2.color = glm::vec3(0.0f, 1.0f, 0.5f);
    createCube(obstacle2, 1.2f);  // Smaller
    
    obstacle3.position = glm::vec3(0.0f, 0.4f, -5.0f);  // Further away from player start
    obstacle3.scale = glm::vec3(1.0f);
    obstacle3.color = glm::vec3(1.0f, 1.0f, 0.0f);
    createCube(obstacle3, 1.0f);  // Smaller
    
    std::cout << "Created " << collisionBoxes.size() << " collision test boxes" << std::endl;
    std::cout << "=== Models Loaded ===\n" << std::endl;
}

// Forward declarations
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// AABB (Axis-Aligned Bounding Box) intersection detection
bool checkIntersection(const GameObject& obj1, const GameObject& obj2) {
    // Calculate world-space bounds for obj1
    glm::vec3 obj1Min = obj1.position + obj1.minBounds * obj1.scale;
    glm::vec3 obj1Max = obj1.position + obj1.maxBounds * obj1.scale;
    
    // Calculate world-space bounds for obj2
    glm::vec3 obj2Min = obj2.position + obj2.minBounds * obj2.scale;
    glm::vec3 obj2Max = obj2.position + obj2.maxBounds * obj2.scale;
    
    // Check if AABBs overlap
    return (obj1Min.x <= obj2Max.x && obj1Max.x >= obj2Min.x) &&
           (obj1Min.y <= obj2Max.y && obj1Max.y >= obj2Min.y) &&
           (obj1Min.z <= obj2Max.z && obj1Max.z >= obj2Min.z);
}

// Mouse callback for camera rotation
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    // Update yaw and pitch based on mouse movement
    // yaw: horizontal rotation (left/right mouse movement)
    //   0° = facing +X (right) ← starting direction
    //   90° = facing +Z (backward)
    //   180° = facing -X (left)
    //   -90° = facing -Z (forward)
    //   -180° = facing -X (left, same as 180°)
    // pitch: vertical rotation (up/down mouse movement)
    //   -89° to +89° (constrained)
    yaw += xoffset;
    pitch += yoffset;
    
    // Constrain pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    
    // Calculate new camera front vector from yaw and pitch
    // This converts the angle representation to a direction vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
    
    // Debug output (uncomment to see yaw calculation)
    // static int debugCount = 0;
    // if (debugCount++ % 60 == 0) {
    //     std::cout << "Mouse moved: xoffset=" << xoffset << ", yoffset=" << yoffset 
    //               << " | New yaw=" << yaw << "°, pitch=" << pitch << "°" << std::endl;
    // }
}

// Scroll callback for zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

// Process input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    const float moveSpeed = 5.0f * deltaTime;
        // Simple movement system based on camera yaw angle
    // W = yaw + 0° (forward), S = yaw + 180° (backward)
    // A = yaw - 90° (left), D = yaw + 90° (right)
    
    bool moving = false;
    float movementAngle = 0.0f;  // Angle relative to camera yaw
    isADMovement = false;  // Reset flag
    
    bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool sPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool aPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    
    // Determine movement angle relative to camera yaw (for movement direction)
    // This is correct and should not be changed
    if (wPressed) {
        movementAngle = 0.0f;  // Forward = camera direction
        moving = true;
    } else if (sPressed) {
        movementAngle = 180.0f;  // Backward = opposite camera direction
        moving = true;
    } else if (aPressed) {
        movementAngle = -90.0f;  // Left = camera left
        isADMovement = true;
        moving = true;
    } else if (dPressed) {
        movementAngle = 90.0f;  // Right = camera right
        isADMovement = true;
        moving = true;
    }
    
    // Calculate movement and rotation
    if (moving) {
        // Calculate movement direction from camera yaw + movement angle
        // This is correct and should not be changed
        float moveAngle = yaw + movementAngle;
        float angleRad = glm::radians(moveAngle);
        glm::vec3 moveDir = glm::vec3(cos(angleRad), 0.0f, sin(angleRad));
        
        // Calculate player rotation separately for model orientation
        // The model needs to face the movement direction, but with an offset
        // because the model's forward direction is rotated relative to the coordinate system
        // 
        // The key insight: offset should be calculated based on movementAngle, not hard-coded per key
        // Pattern observed:
        //   movementAngle = 0° (W)   → offset = -90°
        //   movementAngle = 180° (S)  → offset = -90°  
        //   movementAngle = -90° (A)  → offset = 90°
        //   movementAngle = 90° (D)   → offset = -270° (to avoid normalization to 90° when we want -270°)
        //
        // We can calculate offset from movementAngle:
        float rotationOffset = 0.0f;
        if (std::abs(movementAngle) < 1.0f || std::abs(std::abs(movementAngle) - 180.0f) < 1.0f) {
            // W (0°) or S (180° or -180°): offset = -90°
            rotationOffset = -90.0f;
        } else if (std::abs(movementAngle - (-90.0f)) < 1.0f) {
            // A (-90°): offset = 90°
            rotationOffset = 90.0f;
        } else if (std::abs(movementAngle - 90.0f) < 1.0f) {
            // D (90°): need to check if +90° would normalize incorrectly
            float testWith90 = moveAngle + 90.0f;
            // Normalize to check
            while (testWith90 > 180.0f) testWith90 -= 360.0f;
            while (testWith90 < -180.0f) testWith90 += 360.0f;
            
            // If +90° would give us 90° after normalize, use -270° instead
            if (std::abs(testWith90 - 90.0f) < 1.0f) {
                rotationOffset = -270.0f;
            } else {
                rotationOffset = 90.0f;
            }
        }
        
        playerRotation = moveAngle + rotationOffset;
        
        // Normalize rotation to [-180, 180] range for consistency
        // But preserve -270° for D key when we specifically used -270° offset
        if (!(std::abs(rotationOffset - (-270.0f)) < 1.0f && std::abs(playerRotation - (-270.0f)) < 1.0f)) {
            while (playerRotation > 180.0f) playerRotation -= 360.0f;
            while (playerRotation < -180.0f) playerRotation += 360.0f;
        }
        
        // Debug output: show camera direction, rotation, and movement direction
        static int debugCount = 0;
        if (debugCount++ % 60 == 0) {
            std::cout << "=== Movement Debug ===" << std::endl;
            std::cout << "Camera Yaw: " << yaw << "°" << std::endl;
            std::cout << "Movement Angle Offset: " << movementAngle << "°" << std::endl;
            std::cout << "Move Angle (for movement): " << moveAngle << "°" << std::endl;
            std::cout << "Rotation Offset: " << rotationOffset << "°" << std::endl;
            std::cout << "Player Rotation: " << playerRotation << "°" << std::endl;
            std::cout << "Movement Direction: (" << moveDir.x << ", " << moveDir.y << ", " << moveDir.z << ")" << std::endl;
            std::cout << "====================" << std::endl;
        }
        
        // Calculate new position
        glm::vec3 newPos = player.position + moveDir * moveSpeed;
        
        // Check for collisions before moving
        GameObject tempPlayer = player;
        tempPlayer.position = newPos;
        
        bool canMove = true;
        std::string collisionReason = "";
        
        // Check collision with obstacles
        if (checkIntersection(tempPlayer, obstacle1)) {
            canMove = false;
            collisionReason = "obstacle1";
        } else if (checkIntersection(tempPlayer, obstacle2)) {
            canMove = false;
            collisionReason = "obstacle2";
        } else if (checkIntersection(tempPlayer, obstacle3)) {
            canMove = false;
            collisionReason = "obstacle3";
        }
        
        // Check collision with all collision boxes
        if (canMove) {
            for (const auto& box : collisionBoxes) {
                if (checkIntersection(tempPlayer, box)) {
                    canMove = false;
                    collisionReason = "collision box";
                    break;
                }
            }
        }
        
        if (canMove) {
            player.position = newPos;
        } else {
            // Debug: print collision info (only occasionally to avoid spam)
            static int collisionCount = 0;
            if (collisionCount++ % 60 == 0) {  // Print every 60 frames
                std::cout << "Collision detected with " << collisionReason 
                          << " at position (" << newPos.x << ", " << newPos.y << ", " << newPos.z << ")" << std::endl;
            }
        }
    }
}

// Render a game object
void renderObject(const GameObject& obj, Common::Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!obj.loaded) return;
    
    shader.use();
    
    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.position);
    
    // Rotate player (car) based on movement direction
    if (&obj == &player) {
        // Rotate by playerRotation to face movement direction
        // playerRotation is calculated as: yaw + movementAngle (0°, 180°, -90°, or 90°)
        // No offset - use playerRotation directly to match movement direction
        model = glm::rotate(model, glm::radians(playerRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    model = glm::scale(model, obj.scale);
    
    // Set uniforms
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("objectColor", obj.color);
    shader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 5.0f));
    shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setVec3("viewPos", cameraPos);
    
    // Bind texture if available
    if (obj.hasTexture && obj.textureID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.textureID);
        shader.setInt("texture1", 0);
        shader.setBool("hasTexture", true);
        // Debug: verify texture is bound
        static bool textureDebugPrinted = false;
        if (!textureDebugPrinted && &obj == &player) {
            std::cout << "Player texture bound - ID: " << obj.textureID << std::endl;
            textureDebugPrinted = true;
        }
    } else {
        shader.setBool("hasTexture", false);
        if (&obj == &player) {
            std::cout << "Warning: Player has no texture! hasTexture=" << obj.hasTexture 
                      << ", textureID=" << obj.textureID << std::endl;
        }
    }
    
    // Draw object
    glBindVertexArray(obj.VAO);
    glDrawElements(GL_TRIANGLES, obj.indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Helper function to find shader file
std::string findShaderFile(const std::string& relativePath) {
    // Try multiple possible paths
    std::vector<std::string> paths = {
        relativePath,  // Try as-is first
        "resources/" + relativePath,  // Resources in current directory
        "../" + relativePath,  // One level up
        "../resources/" + relativePath  // Resources one level up
    };
    
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.good()) {
            file.close();
            std::cout << "Found shader at: " << path << std::endl;
            return path;
        }
    }
    
    // If not found, return original (will show error)
    std::cout << "Warning: Could not find shader file: " << relativePath << std::endl;
    return relativePath;
}

int main() {
    // Change to executable directory if possible
    #ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size); // Get required buffer size
    if (size > 0) {
        std::vector<char> buffer(size);
        if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
            std::string execPath = std::string(buffer.data());
            size_t lastSlash = execPath.find_last_of("/");
            if (lastSlash != std::string::npos) {
                std::string execDir = execPath.substr(0, lastSlash);
                if (chdir(execDir.c_str()) == 0) {
                    std::cout << "Changed working directory to: " << execDir << std::endl;
                }
            }
        }
    }
    #endif
    
    // Initialize window
    Common::Window window(SCR_WIDTH, SCR_HEIGHT, "Assignment 3: Loading 3D Model, Camera Following, Object Intersection");
    
    if (window.window == nullptr) {
        return -1;
    }
    
    // Set mouse callbacks
    glfwSetCursorPosCallback(window.window, mouse_callback);
    glfwSetScrollCallback(window.window, scroll_callback);
    
    // Tell GLFW to capture our mouse
    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Load shaders - find the correct path
    std::string vertexPath = findShaderFile("resources/vs/basic.vs");
    std::string fragmentPath = findShaderFile("resources/fs/basic.fs");
    
    Common::Shader shader(vertexPath.c_str(), fragmentPath.c_str());
    
    // Initialize objects
    initializeObjects();
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Render loop
    while (!window.shouldClose()) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Process input
        processInput(window.window);
        
        // Update window title with camera and player rotation info
        static int titleUpdateCount = 0;
        if (titleUpdateCount++ % 10 == 0) {  // Update every 10 frames to avoid too frequent updates
            char title[256];
            snprintf(title, sizeof(title), 
                "Assignment 3 | Camera Yaw: %.1f° | Pitch: %.1f° | Player Rotation: %.1f° | FOV: %.1f°",
                yaw, pitch, playerRotation, fov);
            glfwSetWindowTitle(window.window, title);
        }
        
        // Render
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Create view and projection matrices
        // Camera position follows player, but rotation is controlled by mouse
        glm::vec3 cameraTarget = player.position;
        glm::vec3 currentCameraPos = cameraTarget - cameraFront * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);
        glm::mat4 view = glm::lookAt(currentCameraPos, cameraTarget, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        // Render all objects (order matters for depth)
        // Render ground first
        renderObject(ground, shader, view, projection);
        
        // Render player (Hummer EV) - player is the car now!
        renderObject(player, shader, view, projection);
        
        // Render obstacles
        renderObject(obstacle1, shader, view, projection);
        renderObject(obstacle2, shader, view, projection);
        renderObject(obstacle3, shader, view, projection);
        
        // Render all collision test boxes
        for (const auto& box : collisionBoxes) {
            renderObject(box, shader, view, projection);
        }
        
        // Check collisions (optional: uncomment to see collision messages)
        // if (checkIntersection(player, obstacle1)) {
        //     std::cout << "Collision with obstacle 1!" << std::endl;
        // }
        // if (checkIntersection(player, scene)) {
        //     std::cout << "Collision with Hummer!" << std::endl;
        // }
        
        // Swap buffers and poll events
        window.swapBuffers();
        window.pollEvents();
    }
    return 0;
}
