#pragma once

#include <Common.h>
#include <MaterialManager.h>

class Material {
private:
    friend class MaterialManager;
    friend class AssimpImporter;
    friend class Editor;

    std::string name; // インポート時だけ使う
    std::string pixelShaderCode; // マテリアル固有の HLSL コード
    std::string filePath;

    bool isCustomShader = false;

public:

    DefinedParameters saveParams;

    const std::string& GetPixelShaderCode() { return pixelShaderCode; }
    void SetPixelShaderCode(const std::string& code);

    bool IsCustomShader() { return isCustomShader; }

    bool CanBuild();

    void SetDefaultShader();

    void SaveToJson(const std::filesystem::path& path);
    bool LoadFromJson(const std::filesystem::path& path);
};
