// DefinedParameters.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <map>

class DefinedParameters {
public:
    struct TextureBinding
    {
        UINT slot;              // register(t#)
        std::string path;       // texture file path
    };
private:
    std::map<std::string, float> floats;
    std::map<std::string, DirectX::XMFLOAT4> colors;
    std::map<std::string, TextureBinding> textures;

public:
    std::map<std::string, float>& getFloats() { return floats; }
    std::map<std::string, DirectX::XMFLOAT4>& getColors() { return colors; }
    std::map<std::string, TextureBinding>& getTextures() { return textures; }
    void clearFloats() { return floats.clear(); }
    void clearColors() { return colors.clear(); }
    void clearTextures() { return textures.clear(); }
    void clearAll() { clearFloats(); clearColors(); clearTextures(); }

    void setKey(const std::string& keyname, const float& input) {
        colors.erase(keyname);
        textures.erase(keyname);
        floats[keyname] = input;
    }
    void setKey(const std::string& keyname, const DirectX::XMFLOAT4& input) {
        floats.erase(keyname);
        textures.erase(keyname);
        colors[keyname] = input;
    }
    void setKey(const std::string& keyname, const TextureBinding& input) {
        floats.erase(keyname);
        colors.erase(keyname);
        textures[keyname] = input;
    }
    bool tryGetKey(const std::string& keyname, float* output) {
        if (floats.contains(keyname)) {
            *output = floats[keyname];
            return true;
        }
        return false;
    }
    bool tryGetKey(const std::string& keyname, DirectX::XMFLOAT4* output) {
        if (colors.contains(keyname)) {
            *output = colors[keyname];
            return true;
        }
        return false;
    }
    bool tryGetKey(const std::string& keyname, TextureBinding* output) {
        if (textures.contains(keyname)) {
            *output = textures[keyname];
            return true;
        }
        return false;
    }

    void Merge(DefinedParameters& input)
    {
        for (auto& [key, val] : floats) {
            float output;
            if (input.tryGetKey(key, &output))
            {
                val = output;
            }
        }
        for (auto& [key, val] : colors) {
            DirectX::XMFLOAT4 output;
            if (input.tryGetKey(key, &output))
            {
                val = output;
            }
        }
        for (auto& [key, val] : textures)
        {
            TextureBinding output;
            if (input.tryGetKey(key, &output))
            {
                val.path = output.path;
            }
        }
    }
};
