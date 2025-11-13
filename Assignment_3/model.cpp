#include "model.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>

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
            // Load material file if needed (simplified for now)
        }
    }
    
    file.close();
    
    // Try to load texture
    // Texture paths relative to the model file location
    std::vector<std::string> texturePaths = {
        directory + "/../textures/LP_Firefox_1001_BaseColor.png",
        directory + "/../../textures/LP_Firefox_1001_BaseColor.png",
        "resource/pbr-low-poly-fox-character/textures/LP_Firefox_1001_BaseColor.png",
        "../Assignment 3/resource/pbr-low-poly-fox-character/textures/LP_Firefox_1001_BaseColor.png",
        "../../Assignment 3/resource/pbr-low-poly-fox-character/textures/LP_Firefox_1001_BaseColor.png"
    };
    
    bool textureLoaded = false;
    for (const auto &texPath : texturePaths) {
        unsigned int textureID = TextureFromFile(texPath.c_str(), directory);
        if (textureID != 0) {
            Texture texture;
            texture.id = textureID;
            texture.type = "diffuse";
            texture.path = texPath;
            textures.push_back(texture);
            textureLoaded = true;
            break;
        }
    }
    
    if (!textureLoaded) {
        std::cout << "Warning: Could not load texture for model. Model will render without texture." << std::endl;
    }
    
    if (!vertices.empty()) {
        Mesh mesh(vertices, indices, textures);
        meshes.push_back(mesh);
    }
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
    
    // Try absolute path first, then relative to directory
    std::ifstream testFile(filename);
    if (!testFile.good()) {
        // Try relative to directory
        std::string altPath = directory + "/" + filename;
        testFile.close();
        testFile.open(altPath);
        if (testFile.good()) {
            filename = altPath;
        }
        testFile.close();
    } else {
        testFile.close();
    }
    
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
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
        std::cout << "Texture loaded successfully: " << filename << std::endl;
    } else {
        // Texture failed to load, delete the texture ID
        glDeleteTextures(1, &textureID);
        textureID = 0;
        // Don't print error for every failed attempt, only if all fail
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

