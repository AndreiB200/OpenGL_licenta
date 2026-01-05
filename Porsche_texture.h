#pragma once

#include <glad/glad.h>
//#include "stb_image.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
using namespace std;

class Porsche
{
public:
    struct Texture
    {
        unsigned int albedo, roughness, metallic, normal;
    };
    vector<Texture> loadedTexture;

    void readFromTxtPBR(const char* directory, const char* path)
    {
        string line;
        vector<string> data;
        ifstream dataIn(path);
        while (getline(dataIn, line))
        {
            data.push_back(line);
        }
        unsigned int i = 0;
        while (i < data.size())
        {
            Texture texture;
            texture.albedo = textureFile(data[i].c_str(), directory);
            texture.metallic = textureFile(data[i + 1].c_str(), directory);
            texture.normal = textureFile(data[i + 2].c_str(), directory);
            texture.roughness = textureFile(data[i + 3].c_str(), directory);
            i = i + 4;
            loadedTexture.push_back(texture);
            break;
        }

    }

    unsigned int textureFile(const char* path, const string& directory)
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

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }
};