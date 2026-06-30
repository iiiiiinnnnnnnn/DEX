#include "Material.h"
#include "Graphics.h"
#include <AssetManager.h>

void Material::SetPixelShaderCode(const std::string& code)
{
    if (code == pixelShaderCode)
        return;

    pixelShaderCode = code;
    MaterialManager::Instance().clearCache(filePath);
}

// ビルドできるか試すだけのやつ
bool Material::CanBuild()
{
    if (isCustomShader)
    {
        // カスタムシェーダー
        static Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        static Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        return SUCCEEDED(D3DCompile(
            pixelShaderCode.c_str(),
            pixelShaderCode.size(),
            nullptr,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "ps_5_0",
            D3DCOMPILE_ENABLE_STRICTNESS,
            0,
            psBlob.GetAddressOf(),
            errorBlob.GetAddressOf()
        ));
    }
    else
    {
        // デフォルトシェーダー
        return true;
    }
}

void Material::SetDefaultShader()
{
    std::ifstream ifs("ModelPS.hlsl", std::ios::in | std::ios::binary);
    if (!ifs) return _ASSERT_EXPR(false, L"ModelPS.hlsl is not found");
    std::ostringstream oss;
    oss << ifs.rdbuf();

    std::string pixelShaderCode = oss.str();
    SetPixelShaderCode(pixelShaderCode);
}

void Material::SaveToJson(const std::filesystem::path& path)
{
    filePath = path.string();

    nlohmann::json j;
    j["pixelShaderCode"] = pixelShaderCode;

    j["floats"] = saveParams.getFloats();

    for (auto& [key, val] : saveParams.getColors()) {
        j["colors"][key] = { val.x, val.y, val.z, val.w };
    }

    for (auto& [key, val] : saveParams.getTextures()) {
        j["textures"][key]["slot"] = -1;
        j["textures"][key]["path"] = val.path;
    }

    j["isCustomShader"] = isCustomShader;

    std::ofstream ofs(path);
    ofs << j.dump(4);
}

bool Material::LoadFromJson(const std::filesystem::path& path)
{
    filePath = path.string();

    std::ifstream ifs(path);
    if (!ifs) return false;

    nlohmann::json j;
    try
    {
        ifs >> j;
    }
    catch (...)
    {
        return false;
    }

    pixelShaderCode = j.value("pixelShaderCode", "");

    saveParams.clearAll();

    if (j.contains("floats")) {
        for (auto& [key, val] : j["floats"].items()) {
            saveParams.setKey(key, val.get<float>());
        }
    }

    if (j.contains("colors")) {
        for (auto& [key, val] : j["colors"].items()) {
            saveParams.setKey(key, DirectX::XMFLOAT4(val[0], val[1], val[2], val[3]));
        }
    }

    if (j.contains("textures")) {
        for (auto& [key, val] : j["textures"].items()) {
            saveParams.setKey(key, DefinedParameters::TextureBinding(-1, val["path"]));
        }
    }

    if (j.contains("isCustomShader"))
        isCustomShader = j["isCustomShader"];

    return true;
}
