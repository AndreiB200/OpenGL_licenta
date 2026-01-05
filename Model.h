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

using namespace std;

class Model: public Transform
{
public:
    bool model_loaded = false;

    bool dynamic = false;
    
    vector<Mesh>    meshes;
    vector<Texture_PBR> texture;

    string          directory;
    bool            gammaCorrection;

    unsigned int totalPoly = 0;
    vector<vector<Vertex>> allVertices;
    vector<vector<unsigned int>> allIndices;
    
    string path;
    bool threaded = false;

    void applyMatrix(Shader& shader)
    {
        if (dynamic) 
        {
            glm::mat4 d_model = glm::mat4(1.0f);
            d_model = glm::translate(d_model, position);
            d_model = glm::scale(d_model, size);
            d_model = glm::rotate(d_model, glm::radians(degrees), rotation);
            shader.setMat4("model", d_model);
            glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(d_model)));
            shader.setMat3("normalMatrix", normal);
        }

        else 
        {
            shader.setMat4("model", model);
            glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
            shader.setMat3("normalMatrix", normal);
        }
    }

    void draw(Shader& shader, unsigned int x)
    {
        meshes[x].Draw(shader);
    }
    void draw(Shader& shader)
    {
		applyMatrix(shader);
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);       
    }

    void buildTexture(const char *directory, const char *path)
    {
        texture = textures.texture2DPbr(directory, path);
    }

    Model(string const& paath, bool gamma = false) : gammaCorrection(gamma)
    {
        path = paath;
        std::function<void()> f = std::bind(&Model::loadModel, this);
        
        if (threaded == true)
        {
            threads.addJob(f);
            std::cout << "Thread initializat " << std::endl;
        }
        else
        {
            f();
            loadMeshes();
        }

    }

    void applyData()
    {        
        loadMeshes();
    }

    void setModelDynamic_ON()
    {
        dynamic = true;
    }

    void setModelDynamic_OFF()
    {
        dynamic = false;
    }

private:
    void loadMeshes()
    {
        for (unsigned int i = 0; i < allVertices.size(); i++)
        {
            meshes.push_back(Mesh(allVertices[i], allIndices[i]));
            totalPoly = totalPoly + allVertices[i].size();
        }
        std::cout << "Acest model are " << totalPoly << " poligoane cu ID: " << meshes[0].VAO << std::endl;
        allVertices.clear();
        allVertices.shrink_to_fit();
        allIndices.clear();
        allVertices.shrink_to_fit();
        std::cout << "Modelul este incarcat in GPU !" << std::endl;
    }
    void loadModel()
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
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
};
#endif