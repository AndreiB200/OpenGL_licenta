#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>

#include "Transform.h"
#include "Thread_Pool.h"
#include "Texture.h"
#include "PhysicsEngine.h"


using namespace std;

struct BoundingBox
{
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
};

class Model: virtual public Transform
{
public:
    bool model_loaded = false;

    bool dynamic = true;
    
    vector<Mesh>    meshes; int selectMesh = 0;
    vector<Texture_PBR> texture; int selectTexture = 0;

    string          directory;
    bool            gammaCorrection;

    unsigned int totalPoly = 0;
    int meshNumbers = 0;
    vector<vector<Vertex>> allVertices;
    vector<vector<unsigned int>> allIndices;
    std::vector<JPH::Vec3> joltVertices;

    
    string path;
    bool threaded = false;

    unsigned int textureAlpha = 0;

    BoundingBox boundingBox;
    JPH::BodyID physics_id;
    bool convex = false;
    bool takePhysicsMatrix = false;

    void applyPhysicsMatrix(bool value) { takePhysicsMatrix = value; }

    void applyMatrix(Shader& shader)
    {
        if (dynamic) 
        {
            if (!takePhysicsMatrix)
            {
                resetMatrix();
                move(position);
                rotate_Q(quaternion);
                scale(size);
            }
            else
            {
                resetMatrix();
                PhysicsEngine::getInstance().getModelMatrix(physics_id, position, quaternion);
                move(position);
                rotate_Q(quaternion);
                scale(size);
            }

            shader.setMat4("model", model);
            glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
            shader.setMat3("normalMatrix", normal);
        }

        else 
        {
            shader.setMat4("model", model);
            glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
            shader.setMat3("normalMatrix", normal);
        }
    }

    void drawDebug(Shader& shader)
    {
        applyMatrix(shader);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, texture[selectTexture].albedo);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, texture[selectTexture].metallic);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, texture[selectTexture].normal);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, texture[selectTexture].roughness);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, texture[selectTexture].roughness);
        meshes[selectMesh].Draw(shader);
    }

    void drawSimple(Shader& shader)
    {
        applyMatrix(shader);
        meshes[selectMesh].Draw(shader);
    }

    void draw(Shader& shader)
    {
		applyMatrix(shader);
        for (unsigned int i = 0; i < meshNumbers; i++)
        {
            glDisable(GL_BLEND);
            //shader.setBool("useAlpha", false);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, texture[i].albedo);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, texture[i].metallic);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, texture[i].normal);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, texture[i].roughness);
            /*if (i < 1 && textureAlpha != 0) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, textureAlpha);
                shader.setBool("useAlpha", true);
            }*/
            meshes[i].Draw(shader);
        }
    }

    void drawShadow(Shader& shader)
    {
        applyMatrix(shader);
        for (unsigned int i = 0; i < meshNumbers; i++)
            meshes[i].Draw(shader);
    }

    void buildTexture(const char *directory, const char *path)
    {
        texture = textures.texture2DPbr(directory, path);
    }

    void applyPhysicsAABB(bool dynamic = false)
    {
        if(dynamic)
            physics_id = PhysicsEngine::getInstance().createBodyDynamic(boundingBox.max, position, quaternion);
        else
            physics_id = PhysicsEngine::getInstance().createBodyStatic(boundingBox.max, position, quaternion);
    }

    void applyPhysicsConvexHull()
    {
        physics_id = PhysicsEngine::getInstance().createBodyConvexHull(joltVertices, size, position, quaternion);
    }

    Model(string const& paath, bool _convex = false, bool gamma = false) : gammaCorrection(gamma)
    {
        std::cout << paath << std::endl;
        path = paath;
        std::function<void()> f = std::bind(&Model::loadModel, this);

        convex = _convex;
        
        if (threaded == true)
        {
            Thread_Pool::getInstance().addJob(f);
            std::cout << "Thread initializat " << std::endl;
        }
        else
        {
            f();
            loadMeshes();
        }
        std::cout << std::endl;
    }

    void applyData()
    {        
        loadMeshes();
    }

private:
    void loadMeshes()
    {
        for (unsigned int i = 0; i < allVertices.size(); i++)
        {
            meshes.push_back(Mesh(allVertices[i], allIndices[i]));
            totalPoly = totalPoly + allVertices[i].size();
        }
        if (convex)
        {
            size_t totalVertices = 0;
            for (const auto& meshVertices : allVertices)
            {
                totalVertices += meshVertices.size();
            }
            joltVertices.reserve(totalVertices);
            for (const auto& meshVertices : allVertices)      
            {
                for (const auto& vertex : meshVertices)
                {
                    joltVertices.push_back(JPH::Vec3(vertex.Position.x, vertex.Position.y, vertex.Position.z));
                }
            }
        }
        std::cout << "Acest model are " << totalPoly << " poligoane cu ID: " << meshes[0].VAO << std::endl;
        allVertices.clear();
        allVertices.shrink_to_fit();
        allIndices.clear();
        allVertices.shrink_to_fit();
        std::cout << "Modelul este incarcat in GPU !" << std::endl;
        meshNumbers = meshes.size();
        std::cout << "Box Shape min: " << boundingBox.min.x << ", " << boundingBox.min.y << ", " << boundingBox.min.z << std::endl;
        std::cout << "Box Shape max: " << boundingBox.max.x << ", " << boundingBox.max.y << ", " << boundingBox.max.z << std::endl;
    }
    void loadModel()
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate  |  aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ASSIMP:: !! ERROR !! -> " << importer.GetErrorString() << endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);

        model_loaded = true;
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processVecMesh(mesh, scene);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }    

    void processVecMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            vector.x = mesh->mVertices[i].x; vector.y = mesh->mVertices[i].y; vector.z = mesh->mVertices[i].z;
            getMinMaxAABB(vector);
            vertex.Position = vector;
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x; vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;

                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
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
       
        allVertices.push_back(vertices);
        allIndices.push_back(indices);
        std::cout << allVertices.size() << std::endl;
    }

    void getMinMaxAABB(glm::vec3 v)
    {
        boundingBox.min.x = std::min(boundingBox.min.x, v.x);
        boundingBox.min.y = std::min(boundingBox.min.y, v.y);
        boundingBox.min.z = std::min(boundingBox.min.z, v.z);

        boundingBox.max.x = std::max(boundingBox.max.x, v.x);
        boundingBox.max.y = std::max(boundingBox.max.y, v.y);
        boundingBox.max.z = std::max(boundingBox.max.z, v.z);
    }
};
#endif