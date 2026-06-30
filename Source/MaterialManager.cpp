// MaterialManager.cpp
#include "MaterialManager.h"
#include <Graphics.h>
#include <AssetManager.h>
#include <Logger.h>
#include "Material.h"

void MaterialManager::Initialize()
{
    // デフォルト定数バッファ
    GpuResourceUtils::CreateConstantBuffer(Graphics::Instance().GetDevice(),
        sizeof(CbDefaultMaterial), defaultMaterialConstantBuffer.GetAddressOf());

    // すべてのマテリアルをビルドしてキャッシュ
    clearAllCache();
    cacheAllMaterials();
}

void MaterialManager::cacheAllMaterials()
{
    for (auto path : AssetManager::Instance().ScanAssets(".mat")) {
		getCache(path.string()); // ビルドしてキャッシュ
    }
}

MaterialCache MaterialManager::build(Material& material)
{
    auto device = Graphics::Instance().GetDevice();
    auto& pixelShaderCode = material.GetPixelShaderCode();

    MaterialCache cache = {};
	cache.isCustomShader = material.IsCustomShader();
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

    do
    {
        if (material.IsCustomShader())
        {
            // カスタムシェーダー

            Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

            // コンパイル
            hr = D3DCompile(
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
            );
            if (hr != S_OK)
            {
                if (errorBlob) LOG("%s", static_cast<const char*>(errorBlob->GetBufferPointer()));
                break;
            }

            // リフレクション
            Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflector;
            hr = D3DReflect(
                psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                IID_ID3D11ShaderReflection, reinterpret_cast<void**>(reflector.GetAddressOf()));
            if (hr != S_OK) break;

            D3D11_SHADER_DESC shaderDesc;
            reflector->GetDesc(&shaderDesc);

            // カスタム定数バッファを作る
            for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
            {
                auto* cb = reflector->GetConstantBufferByIndex(i);
                D3D11_SHADER_BUFFER_DESC cbDesc;
                cb->GetDesc(&cbDesc);

                for (UINT v = 0; v < cbDesc.Variables; ++v)
                {
                    auto* var = cb->GetVariableByIndex(v);
                    D3D11_SHADER_VARIABLE_DESC varDesc;
                    var->GetDesc(&varDesc);

                    auto* type = var->GetType();
                    D3D11_SHADER_TYPE_DESC typeDesc;
                    type->GetDesc(&typeDesc);

                    if (typeDesc.Class == D3D_SVC_SCALAR &&
                        typeDesc.Type == D3D_SVT_FLOAT)
                    {
                        if (std::string(varDesc.Name) != "shadowTexelSize") {
                            cache.reflectedParams.setKey(varDesc.Name, 0);
                        }
                    }
                    else if (typeDesc.Class == D3D_SVC_VECTOR &&
                             typeDesc.Type == D3D_SVT_FLOAT &&
                             typeDesc.Columns == 4)
                    {
                        if (std::string(varDesc.Name) != "camera_position") {
                            cache.reflectedParams.setKey(varDesc.Name, DirectX::XMFLOAT4(0, 0, 0, 0));
                        }
                    }

                    // すべて小文字にする
                    std::string lowered = cbDesc.Name;
                    for (int i = 0; i < lowered.size(); i++) {
                        lowered[i] = std::tolower(lowered[i]);
                    }

                    // リフレクションでmaterial(b3)のパラメーター取る
                    if (lowered == "material")
                    {
                        cache.materialCBSize = cbDesc.Size;
                        cache.materialCpuBuffer.resize(cache.materialCBSize);

                        MaterialCache::ReflectedCBVar cbvar = {};
                        cbvar.offset = varDesc.StartOffset;
                        cbvar.size = varDesc.Size;

                        cache.materialCBVars[varDesc.Name] = cbvar;
                    }
                }
            }
            for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
            {
                D3D11_SHADER_INPUT_BIND_DESC bindDesc;
                reflector->GetResourceBindingDesc(i, &bindDesc);

                if (bindDesc.Type == D3D_SIT_TEXTURE)
                {
                    if (std::string(bindDesc.Name) != "shadowMap")
                    {
                        DefinedParameters::TextureBinding binding;
                        binding.slot = bindDesc.BindPoint; // t0, t1, t2...
                        binding.path = "";
                        cache.reflectedParams.setKey(bindDesc.Name, binding);
                    }
                }
            }

            // マージ(material user param -> cache reflected param)
            cache.reflectedParams.Merge(material.saveParams);

            // ピクセルシェーダー作成
            hr = device->CreatePixelShader(
                psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
            if (hr != S_OK) break;
        }
        else
        {
            // デフォルトシェーダー

            cache.reflectedParams.setKey("diffuseColor", { 1,1,1,1 });
            cache.reflectedParams.setKey("specularPower", 0);
            cache.reflectedParams.setKey("diffuseMap", { 1, "" });
            cache.reflectedParams.setKey("normalMap", { 2, "" });

            // マージ(material user param -> cache reflected param)
            cache.reflectedParams.Merge(material.saveParams);

            // ピクセルシェーダー作成
            hr = GpuResourceUtils::LoadPixelShader(
                Graphics::Instance().GetDevice(), "ModelPS._cso", &pixelShader);
            if (hr != S_OK) break;
        }
    } while (false);

    // エラーシェーダー

    if (hr != S_OK)
    {
        // ピクセルシェーダー作成
        HRESULT hr = GpuResourceUtils::LoadPixelShader(device, "ModelErrorPS._cso", &pixelShader);
        _ASSERT_EXPR(SUCCEEDED(hr), L"ModelErrorPS._cso is not found");
    }

    // 完成したピクセルシェーダーを格納
    cache.setPixelShader(pixelShader.Get());

    return cache;
}

void MaterialManager::setCache(const MaterialCache& cache, const std::string& path)
{
    caches[hashCode(path)] = cache;
}

MaterialCache MaterialManager::getCache(const std::string& path)
{
    if (caches.contains(hashCode(path))) {
        return caches[hashCode(path)];
    }
    else {
		// キャッシュなしの場合はビルドしてキャッシュする一連の動作
        Material mat;
        mat.LoadFromJson(path);
        MaterialCache cache = build(mat);
		clearCache(path);
        setCache(cache, path);
        return cache;
    }
}

void MaterialManager::clearCache(const std::string& path)
{
    caches.erase(hashCode(path));
}

void MaterialManager::clearAllCache()
{
    caches.clear();
}


