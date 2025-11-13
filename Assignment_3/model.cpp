#include "model.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <map>
#include <cctype>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

// Mesh implementation
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // Vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    
    // Vertex texture coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    glBindVertexArray(0);
}

void Mesh::Draw(unsigned int shaderID) {
    // Bind textures
    if (!textures.empty()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0].id);
        glUniform1i(glGetUniformLocation(shaderID, "texture_diffuse1"), 0);
    } else {
        // Debug: check if textures are empty
        static bool warned = false;
        if (!warned) {
            std::cout << "Warning: Mesh has no textures to bind!" << std::endl;
            warned = true;
        }
    }
    
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glActiveTexture(GL_TEXTURE0);
}

// Model implementation
Model::Model(const char *path) {
    boundingBoxMin = glm::vec3(FLT_MAX);
    boundingBoxMax = glm::vec3(-FLT_MAX);
    loadModel(std::string(path));
}

void Model::loadModel(std::string path) {
    size_t lastSlash = path.find_last_of("/\\");
    directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : ".";
    
    // Normalize directory path
    if (directory.empty()) {
        directory = ".";
    }
    
    loadOBJ(path);
}

void Model::loadOBJ(std::string path) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "ERROR: Failed to open OBJ file: " << path << std::endl;
        std::cout << "Current working directory might be wrong." << std::endl;
        std::cout << "Please ensure the executable is run from the build/Assignment 3/ directory." << std::endl;
        return;
    }
    std::cout << "Successfully opened OBJ file: " << path << std::endl;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
            
            // Update bounding box
            boundingBoxMin.x = std::min(boundingBoxMin.x, pos.x);
            boundingBoxMin.y = std::min(boundingBoxMin.y, pos.y);
            boundingBoxMin.z = std::min(boundingBoxMin.z, pos.z);
            boundingBoxMax.x = std::max(boundingBoxMax.x, pos.x);
            boundingBoxMax.y = std::max(boundingBoxMax.y, pos.y);
            boundingBoxMax.z = std::max(boundingBoxMax.z, pos.z);
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
            std::vector<std::string> vertexTokens;
            std::string vertexToken;
            while (iss >> vertexToken) {
                vertexTokens.push_back(vertexToken);
            }

            if (vertexTokens.size() < 3) {
                continue; // Not enough vertices to form a face
            }

            auto parseFaceToken = [&](const std::string &face) {
                std::stringstream ss(face);
                std::string token;
                int posIdx = -1, texIdx = -1, normIdx = -1;

                if (std::getline(ss, token, '/')) {
                    if (!token.empty()) posIdx = std::stoi(token) - 1;
                }
                if (std::getline(ss, token, '/')) {
                    if (!token.empty()) texIdx = std::stoi(token) - 1;
                }
                if (std::getline(ss, token, '/')) {
                    if (!token.empty()) normIdx = std::stoi(token) - 1;
                }

                Vertex vertex;
                if (posIdx >= 0 && posIdx < static_cast<int>(positions.size())) {
                    vertex.Position = positions[posIdx];
                }
                if (texIdx >= 0 && texIdx < static_cast<int>(texCoords.size())) {
                    vertex.TexCoords = texCoords[texIdx];
                } else {
                    vertex.TexCoords = glm::vec2(0.0f);
                }
                if (normIdx >= 0 && normIdx < static_cast<int>(normals.size())) {
                    vertex.Normal = normals[normIdx];
                } else {
                    vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                return vertex;
            };

            // Parse all vertices in the face
            std::vector<Vertex> faceVertices;
            faceVertices.reserve(vertexTokens.size());
            for (const auto &token : vertexTokens) {
                faceVertices.push_back(parseFaceToken(token));
            }

            // Triangulate polygon using fan method
            for (size_t i = 1; i + 1 < faceVertices.size(); ++i) {
                unsigned int baseIndex = vertices.size();
                vertices.push_back(faceVertices[0]);
                vertices.push_back(faceVertices[i]);
                vertices.push_back(faceVertices[i + 1]);

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 2);
            }
        }
        else if (type == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;
            // Load material file
            std::string mtlPath = directory + "/" + mtlFile;
            std::cout << "Found mtllib: " << mtlFile << ", loading from: " << mtlPath << std::endl;
            loadMTL(mtlPath);
        }
    }
    
    file.close();
    
    // Use textures from materials if available, otherwise try default texture paths
    if (textures.empty() && !materials.empty()) {
        std::cout << "Found " << materials.size() << " materials, loading textures..." << std::endl;
        // Use all materials' textures
        for (const auto& mat : materials) {
            std::cout << "  Material: " << mat.first << " has " << mat.second.size() << " textures" << std::endl;
            textures.insert(textures.end(), mat.second.begin(), mat.second.end());
        }
        std::cout << "Total textures loaded: " << textures.size() << std::endl;
    } else if (textures.empty()) {
        std::cout << "Warning: No materials found and no textures loaded!" << std::endl;
        std::cout << "Attempting to load default texture from textures folder..." << std::endl;
        // Try to load a default texture if no materials found
        std::vector<std::string> defaultTexturePaths = {
            directory + "/../textures/Textures_color.png",
            directory + "/../textures/280z_CarPaint_AO.png",
            directory + "/../textures/SSR_Color_alternative.png"
        };
        for (const auto& texPath : defaultTexturePaths) {
            unsigned int textureID = TextureFromFile(texPath.c_str(), directory);
            if (textureID != 0) {
                Texture texture;
                texture.id = textureID;
                texture.type = "diffuse";
                texture.path = texPath;
                textures.push_back(texture);
                std::cout << "Loaded default texture: " << texPath << std::endl;
                break;
            }
        }
    }
    
    if (!vertices.empty()) {
        std::cout << "Creating mesh with " << textures.size() << " textures and " << vertices.size() << " vertices" << std::endl;
        Mesh mesh(vertices, indices, textures);
        meshes.push_back(mesh);
    }
}

void Model::loadMTL(std::string path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        // Try alternative paths
        std::string mtlFileName = path.substr(path.find_last_of("/\\") + 1);
        std::vector<std::string> altPaths = {
            directory + "/" + mtlFileName,
            "../" + path,
            "../../" + path
        };
        std::cout << "MTL file not found at: " << path << ", trying alternatives..." << std::endl;
        for (const auto& altPath : altPaths) {
            std::cout << "  Trying: " << altPath << std::endl;
            file.open(altPath);
            if (file.is_open()) {
                path = altPath;
                std::cout << "  Found at: " << altPath << std::endl;
                break;
            }
        }
        if (!file.is_open()) {
            std::cout << "Warning: Could not open MTL file: " << path << std::endl;
            return;
        }
    }
    
    std::cout << "Loading MTL file: " << path << std::endl;
    std::cout << "Directory: " << directory << std::endl;
    std::string currentMaterial;
    std::string line;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "newmtl") {
            iss >> currentMaterial;
            materials[currentMaterial] = std::vector<Texture>();
        }
        else if (type == "map_Kd" || type == "map_Ks" || type == "map_Ka") {
            std::string texPath;
            iss >> texPath;
            
            // Skip if empty
            if (texPath.empty()) {
                continue;
            }
            
            // Remove Windows absolute path if present (extract just filename)
            size_t lastSlash = texPath.find_last_of("/\\");
            if (lastSlash != std::string::npos && lastSlash < texPath.length() - 1) {
                texPath = texPath.substr(lastSlash + 1);
            }
            
            std::cout << "  Processing texture: " << texPath << " for material: " << currentMaterial << std::endl;
            
            // Try multiple paths for texture
            std::vector<std::string> texturePaths;
            
            // Extract base name (filename without extension)
            size_t dotPos = texPath.find_last_of('.');
            std::string baseName = (dotPos != std::string::npos) ? texPath.substr(0, dotPos) : texPath;
            
            // Normalize directory path (remove trailing slashes)
            std::string dir = directory;
            while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
                dir.pop_back();
            }
            
            // Try paths in order of priority:
            // Note: stb_image doesn't support .dds, so we try .png versions first
            // 1. Textures subfolder with PNG (convert .dds to .png) - highest priority
            texturePaths.push_back(dir + "/Textures/" + baseName + ".png");
            // 2. Textures folder (parent) with PNG
            texturePaths.push_back(dir + "/../textures/" + baseName + ".png");
            // 3. Same directory with PNG
            texturePaths.push_back(dir + "/" + baseName + ".png");
            // 4. Textures subfolder with original extension (skip .dds, stb_image doesn't support it)
            if (dotPos != std::string::npos) {
                std::string ext = texPath.substr(dotPos);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".dds") {
                    texturePaths.push_back(dir + "/Textures/" + texPath);
                }
            }
            // 5. Textures folder (parent) with original extension (skip .dds)
            if (dotPos != std::string::npos) {
                std::string ext = texPath.substr(dotPos);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".dds") {
                    texturePaths.push_back(dir + "/../textures/" + texPath);
                }
            }
            // 6. Same directory with original extension (skip .dds)
            if (dotPos != std::string::npos) {
                std::string ext = texPath.substr(dotPos);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".dds") {
                    texturePaths.push_back(dir + "/" + texPath);
                }
            }
            // 7. Same directory with BMP
            texturePaths.push_back(dir + "/" + baseName + ".bmp");
            texturePaths.push_back(dir + "/../textures/" + baseName + ".bmp");
            texturePaths.push_back(dir + "/Textures/" + baseName + ".bmp");
            // 8. Same directory with TGA
            texturePaths.push_back(dir + "/" + baseName + ".tga");
            texturePaths.push_back(dir + "/../textures/" + baseName + ".tga");
            texturePaths.push_back(dir + "/Textures/" + baseName + ".tga");
            // 9. Try with different case variations (for Windows)
            std::string baseNameLower = baseName;
            std::transform(baseNameLower.begin(), baseNameLower.end(), baseNameLower.begin(), ::tolower);
            texturePaths.push_back(dir + "/Textures/" + baseNameLower + ".png");
            texturePaths.push_back(dir + "/../textures/" + baseNameLower + ".png");
            texturePaths.push_back(dir + "/" + baseNameLower + ".png");
            
            bool textureLoaded = false;
            for (const auto& texFile : texturePaths) {
                std::cout << "    Trying: " << texFile << std::endl;
                unsigned int textureID = TextureFromFile(texFile.c_str(), directory);
                if (textureID != 0) {
                    Texture texture;
                    texture.id = textureID;
                    texture.type = (type == "map_Kd") ? "diffuse" : ((type == "map_Ks") ? "specular" : "ambient");
                    texture.path = texFile;
                    materials[currentMaterial].push_back(texture);
                    textureLoaded = true;
                    std::cout << "    ✓ Loaded texture: " << texFile << " for material: " << currentMaterial << std::endl;
                    break;
                }
            }
            
            if (!textureLoaded) {
                std::cout << "    ✗ Warning: Could not load texture: " << texPath << " for material: " << currentMaterial << std::endl;
                std::cout << "      Tried " << texturePaths.size() << " paths" << std::endl;
            }
        }
    }
    
    file.close();
    std::cout << "Loaded " << materials.size() << " materials from MTL file." << std::endl;
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
    
    // Normalize path separators for Windows (convert \ to /)
    std::replace(filename.begin(), filename.end(), '\\', '/');
    
    // Remove double slashes
    size_t pos = 0;
    while ((pos = filename.find("//", pos)) != std::string::npos) {
        filename.replace(pos, 2, "/");
    }
    
    // Try the path as-is first
    std::ifstream testFile(filename);
    if (testFile.good()) {
        testFile.close();
    } else {
        testFile.close();
        // File doesn't exist at this path
        return 0;
    }
    
    unsigned int textureID = 0;
    
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data && width > 0 && height > 0 && nrComponents > 0) {
        glGenTextures(1, &textureID);
        
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            stbi_image_free(data);
            return 0; // Unsupported format
        }
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        std::cout << "Texture loaded successfully: " << filename << std::endl;
    } else {
        if (data) {
            stbi_image_free(data);
        }
        // Texture failed to load, return 0
        textureID = 0;
    }
    
    return textureID;
}

void Model::Draw(unsigned int shaderID) {
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw(shaderID);
    }
}

// Stub implementations for unused methods
void Model::processNode(void *node, void *scene) {}
Mesh Model::processMesh(void *mesh, void *scene) { return Mesh({}, {}, {}); }
std::vector<Texture> Model::loadMaterialTextures(void *mat, int type, std::string typeName) { return {}; }

