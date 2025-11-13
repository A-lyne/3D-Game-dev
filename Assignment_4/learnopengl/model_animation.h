#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <learnopengl/assimp_glm_helpers.h>
#include <learnopengl/animdata.h>

using namespace std;

// Helper to correctly read aiBone fields with packing mismatch
struct aiBoneHelper {
    static bool TryGetPackedWeights(const aiBone* bone, unsigned int& outNumWeights, const aiVertexWeight*& outWeights)
    {
        const unsigned char* base = reinterpret_cast<const unsigned char*>(bone);

        // Attempt to read packed fields (32-bit length + pointer) used by some Assimp builds
        unsigned int packedNum = *reinterpret_cast<const unsigned int*>(base + 1028);
        const aiVertexWeight* packedPtr = *reinterpret_cast<const aiVertexWeight* const*>(base + 1032);

        if (packedPtr != nullptr && packedNum != 0)
        {
            outNumWeights = packedNum;
            outWeights = packedPtr;
            return true;
        }

        return false;
    }
};

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
	
	

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    
	auto& GetBoneInfoMap() { return m_BoneInfoMap; }
	int& GetBoneCount() { return m_BoneCounter; }
	

private:

	std::map<string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        std::cout << "  loadModel: Reading file with Assimp..." << std::endl;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        std::cout << "  loadModel: File read successfully" << std::endl;
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        std::cout << "  loadModel: Processing nodes..." << std::endl;
        processNode(scene->mRootNode, scene);
        std::cout << "  loadModel: Done" << std::endl;
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

	void SetVertexBoneDataToDefault(Vertex& vertex)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.m_BoneIDs[i] = -1;
			vertex.m_Weights[i] = 0.0f;
		}
	}


	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::cout << "    processMesh: Processing mesh with " << mesh->mNumVertices << " vertices" << std::endl;
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			SetVertexBoneDataToDefault(vertex);
			vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
			vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
			
			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		ExtractBoneWeightForVertices(vertices,mesh,scene);

		return Mesh(vertices, indices, textures);
	}

	void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_BoneIDs[i] < 0)
			{
				vertex.m_Weights[i] = weight;
				vertex.m_BoneIDs[i] = boneID;
				break;
			}
		}
	}


	void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
		std::cout << "      ExtractBoneWeightForVertices: Processing " << mesh->mNumBones << " bones" << std::endl;
		auto& boneInfoMap = m_BoneInfoMap;
		int& boneCount = m_BoneCounter;

		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			std::cout << "        Bone " << boneIndex << "/" << mesh->mNumBones << std::endl;
			int boneID = -1;
			// Fix for aiString packing: data starts at offset 4
			const char* boneNamePtr = reinterpret_cast<const char*>(&mesh->mBones[boneIndex]->mName) + 4;
			std::string boneName = boneNamePtr;
			std::cout << "          Name: " << boneName << std::endl;
			
			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = boneCount;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				boneInfoMap[boneName] = newBoneInfo;
				boneID = boneCount;
				boneCount++;
			}
			else
			{
				boneID = boneInfoMap[boneName].id;
			}
			assert(boneID != -1);
            std::cout << "          Getting weights..." << std::endl;
            unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;
            const aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;

            if ((!weights || numWeights == 0) &&
                aiBoneHelper::TryGetPackedWeights(mesh->mBones[boneIndex], numWeights, weights))
            {
                std::cout << "          Using packed Assimp bone layout" << std::endl;
            }

            std::cout << "          NumWeights: " << numWeights << std::endl;
            std::cout << "          Weights pointer: " << (void*)weights << std::endl;

            if (!weights || numWeights == 0)
            {
                std::cout << "ERROR: Bone weights unavailable for bone '" << boneName << "'" << std::endl;
                continue;

			}
			std::cout << "          Processing " << numWeights << " weights..." << std::endl;

            for (unsigned int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				if (weightIndex < 3) {
					std::cout << "            Weight " << weightIndex << ": accessing..." << std::endl;
				}
				int vertexId = weights[weightIndex].mVertexId;
				if (weightIndex < 3) {
					std::cout << "              vertexId=" << vertexId << std::endl;
				}
				float weight = weights[weightIndex].mWeight;
				if (weightIndex < 3) {
					std::cout << "              weight=" << weight << std::endl;
				}
				if (vertexId < 0 || vertexId >= vertices.size()) {
					std::cout << "ERROR: Invalid vertexId " << vertexId << " (vertices.size()=" << vertices.size() << ")" << std::endl;
					continue;
				}
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
			std::cout << "          Done with bone " << boneIndex << std::endl;
		}
	}


	unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false)
	{
		string filename = string(path);
		filename = directory + '/' + filename;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
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
		}
	else
	{
		std::cout << "Texture failed to load at path: " << filename << std::endl;
		stbi_image_free(data);
	}

		return textureID;
	}
    
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            // Fix for aiString packing mismatch: Assimp was compiled with 32-bit length field
            // instead of 64-bit size_t, so the string data starts at offset 4, not offset 8
            const char* texturePath = reinterpret_cast<const char*>(&str) + 4;
            
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), texturePath) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(texturePath, this->directory);
                texture.type = typeName;
                texture.path = texturePath;
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }
};



#endif
