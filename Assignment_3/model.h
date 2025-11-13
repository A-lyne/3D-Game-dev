#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <map>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;
    
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void Draw(unsigned int shaderID);
    
private:
    unsigned int VBO, EBO;
    void setupMesh();
};

class Model {
public:
    Model(const char *path);
    void Draw(unsigned int shaderID);
    glm::vec3 getBoundingBoxMin() const { return boundingBoxMin; }
    glm::vec3 getBoundingBoxMax() const { return boundingBoxMax; }
    
private:
    std::vector<Mesh> meshes;
    std::string directory;
    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    
    void loadModel(std::string path);
    void processNode(void *node, void *scene);
    Mesh processMesh(void *mesh, void *scene);
    std::vector<Texture> loadMaterialTextures(void *mat, int type, std::string typeName);
    unsigned int TextureFromFile(const char *path, const std::string &directory);
    
    // Simple OBJ loader
    void loadOBJ(std::string path);
    void loadMTL(std::string path);
    
    // Material storage
    std::map<std::string, std::vector<Texture>> materials;
};

