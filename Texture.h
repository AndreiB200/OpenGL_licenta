#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <functional>
#include <mutex>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

struct Texture_PBR
{
    unsigned int albedo, roughness, metallic, normal;
};
struct Texture_Values
{
    glm::vec3 albedo = glm::vec3(0.7f); float metallic = 0.0f; float roughness = 0.0f;
};

struct data_texture
{
    unsigned char* data;
    int width, height, nrComponents;
    bool done = false;
};

struct data_texture_f
{
    float* data;
    int width, height, nrComponents;
    bool done = false;
};

class Texture
{
private:
    bool parallel = true;

    void data_loading(data_texture& data, const char* path, const string& directory)
    {
        string filename = string(path);
        filename = directory + '/' + filename;

        int width, height, nrComponents;
        data.data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

        data.width = width; data.height = height; data.nrComponents = nrComponents;
        data.done = true;
    }

    void GLparameter2D()
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

public:
    vector<Texture_PBR> texture2DPbr(const char* directory, const char* path)
    {
        vector<Texture_PBR> loadedTexture;
        string line;
        vector<string> dataPath;
        ifstream dataIn(path);
        while (getline(dataIn, line))
        {
            dataPath.push_back(line);
        }

        std::vector<data_texture> tex_info(dataPath.size());

        if (parallel == true)
        {
            for (int i = 0; i < tex_info.size(); i++)
                threads.addJob(std::bind(&Texture::data_loading, this, std::ref(tex_info[i]), dataPath[i].c_str(), directory));

            threads.wait();
        }
        else
        {
            for (int i = 0; i < tex_info.size(); i++)
            {
                data_loading(tex_info[i], dataPath[i].c_str(), directory);
            }
        }

        for (int i = 0; i < 1;)
        {
            Texture_PBR texture;
            texture.albedo      = texture2Dfile(tex_info[i]);
            texture.metallic    = texture2Dfile(tex_info[i+1]);
            texture.normal      = texture2Dfile(tex_info[i+2]);
            texture.roughness   = texture2Dfile(tex_info[i+3]);
            i = i + 4;
            loadedTexture.push_back(texture);
        }

        tex_info.clear();
        return loadedTexture;
    }

    unsigned int texture2Dfile(data_texture& data)
    {
        unsigned int textureID = 0;

        if (data.data)
        {
            GLenum format;
            if (data.nrComponents == 1)
                format = GL_RED;
            else if (data.nrComponents == 3)
                format = GL_RGB;
            else if (data.nrComponents == 4)
                format = GL_RGBA;

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, data.width, data.height, 0, format, GL_UNSIGNED_BYTE, data.data);
            glGenerateMipmap(GL_TEXTURE_2D);

            GLparameter2D();

            stbi_image_free(data.data);
        }
        else
        {
            cout << "Texture failed to load" << std::endl;
            stbi_image_free(data.data);
        }

        return textureID;
    }

	unsigned int texture2Dfile(const char* path, const string& directory)
	{
        string filename = string(path);
        filename = directory + '/' + filename;

        unsigned int textureID = 0;

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

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            GLparameter2D();

            stbi_image_free(data);
        }
        else
        {
            cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
	}

    unsigned int texture2D_Empty(unsigned int size_x, unsigned int size_y, string type)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        GLenum format;
        if (type == "BGR")
            format = GL_RED;
        else if (type == "RGB" || type == "JPG")
            format = GL_RGB;
        else if (type == "RGBA" || type == "PNG")
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, size_x, size_y, 0, format, GL_UNSIGNED_BYTE, nullptr);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLparameter2D();

        return textureID;
    }

    unsigned int textureCubeMap_empty(unsigned int resolution)
    {
        unsigned int cubeMap = 0;
        glGenTextures(1, &cubeMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
        for (unsigned int i = 0; i < 6; i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, resolution, resolution, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return cubeMap;
    }

    unsigned int texture_HDR(const char* path, string& directory)
    {
        string filename = string(path);
        filename = directory + '/' + filename;

        unsigned int hdrTexture = 0;

        int width, height, nrComponents;
        float* data = stbi_loadf(filename.c_str(), &width, &height, &nrComponents, 0);

        if (data)
        {
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load HDR file !" << std::endl;
            stbi_image_free(data);
        }

        return hdrTexture;
    }
};
Texture textures;