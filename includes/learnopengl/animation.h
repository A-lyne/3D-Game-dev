#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <learnopengl/bone.h>
#include <functional>
#include <learnopengl/animdata.h>
#include <learnopengl/model_animation.h>
#include <iostream>

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

class Animation
{
public:
    Animation()
        : m_Duration(0.0f)
        , m_TicksPerSecond(0)
        , m_IsValid(false)
    {
    }

    Animation(const std::string& animationPath, Model* model)
        : m_Duration(0.0f)
        , m_TicksPerSecond(0)
        , m_IsValid(false)
    {
        if (!model)
        {
            std::cerr << "ERROR::ANIMATION:: Model pointer is null for animation '" << animationPath << "'" << std::endl;
            return;
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
        if (!scene || !scene->mRootNode)
        {
            std::cerr << "ERROR::ANIMATION:: Failed to load animation '" << animationPath
                      << "': " << importer.GetErrorString() << std::endl;
            return;
        }

        if (scene->mNumAnimations == 0)
        {
            std::cerr << "ERROR::ANIMATION:: No animations found in file '" << animationPath << "'" << std::endl;
            return;
        }

        auto animation = scene->mAnimations[0];
        if (!animation)
        {
            std::cerr << "ERROR::ANIMATION:: Animation data is null in file '" << animationPath << "'" << std::endl;
            return;
        }

        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;
        aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
        globalTransformation = globalTransformation.Inverse();
        ReadHierarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);
        m_IsValid = true;
    }

	~Animation()
	{
	}

	Bone* FindBone(const std::string& name)
	{
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Bone& Bone)
			{
				return Bone.GetBoneName() == name;
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return &(*iter);
	}

	
    inline float GetTicksPerSecond() const { return m_TicksPerSecond; }
    inline float GetDuration() const { return m_Duration; }
    inline const AssimpNodeData& GetRootNode() const { return m_RootNode; }
    inline const std::map<std::string,BoneInfo>& GetBoneIDMap() const
	{ 
		return m_BoneInfoMap;
	}

    inline bool IsValid() const { return m_IsValid; }

private:
	void ReadMissingBones(const aiAnimation* animation, Model& model)
	{
        if (!animation)
        {
            std::cerr << "ERROR::ANIMATION:: ReadMissingBones received null animation" << std::endl;
            return;
        }

		int size = animation->mNumChannels;

		auto& boneInfoMap = model.GetBoneInfoMap();//getting m_BoneInfoMap from Model class
		int& boneCount = model.GetBoneCount(); //getting the m_BoneCounter from Model class

		//reading channels(bones engaged in an animation and their keyframes)
		for (int i = 0; i < size; i++)
		{
            auto channel = animation->mChannels[i];
            if (!channel)
            {
                std::cerr << "ERROR::ANIMATION:: Missing channel data for bone index " << i << std::endl;
                continue;
            }
			// Fix for aiString packing: data starts at offset 4, not offset 8
			const char* boneNamePtr = reinterpret_cast<const char*>(&channel->mNodeName) + 4;
			std::string boneName = boneNamePtr;

			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				boneInfoMap[boneName].id = boneCount;
				boneCount++;
			}
            m_Bones.push_back(Bone(boneNamePtr,
                boneInfoMap[boneName].id, channel));
		}

		m_BoneInfoMap = boneInfoMap;
	}

	void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		// Fix for aiString packing: data starts at offset 4, not offset 8
		dest.name = reinterpret_cast<const char*>(&src->mName) + 4;
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHierarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}
    float m_Duration;
    int m_TicksPerSecond;
	std::vector<Bone> m_Bones;
	AssimpNodeData m_RootNode;
	std::map<std::string, BoneInfo> m_BoneInfoMap;
    bool m_IsValid;
};

