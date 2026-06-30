#include "MaterialCache.h"
#include "MaterialManager.h"
#include <Graphics.h>

void MaterialCache::Apply(std::vector<ID3D11Buffer*>& cbuffers)
{
    auto dc = Graphics::Instance().GetDeviceContext();
    dc->PSSetShader(getPixelShader(), nullptr, 0);

    updateCBuffers(cbuffers);
}

void MaterialCache::updateCBuffers(std::vector<ID3D11Buffer*>& cbuffers)
{
    auto device = Graphics::Instance().GetDevice();
    auto dc = Graphics::Instance().GetDeviceContext();

    if (isCustomShader)
    {
        // CB 未作成なら作る
        if (!materialConstantBuffer)
        {
            GpuResourceUtils::CreateConstantBuffer(device,
                (materialCBSize + 15) & ~15, materialConstantBuffer.GetAddressOf(), true);

            // CPU バッファ初期化
            memset(materialCpuBuffer.data(), 0, materialCpuBuffer.size());
        }

        for (auto& [name, value] : reflectedParams.getFloats())
        {
            if (materialCBVars.contains(name))
            {
                memcpy(materialCpuBuffer.data() + materialCBVars[name].offset, &value, sizeof(float));
            }
        }
        for (auto& [name, value] : reflectedParams.getColors())
        {
            if (materialCBVars.contains(name))
            {
                memcpy(materialCpuBuffer.data() + materialCBVars[name].offset, &value, sizeof(DirectX::XMFLOAT4));
            }
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(dc->Map(materialConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, materialCpuBuffer.data(), materialCpuBuffer.size());
            dc->Unmap(materialConstantBuffer.Get(), 0);
        }

        cbuffers.push_back(materialConstantBuffer.Get());
    }
    else
    {
        MaterialManager::CbDefaultMaterial cbDefaultMat;
        reflectedParams.tryGetKey("diffuseColor", &cbDefaultMat.diffuseColor);
        reflectedParams.tryGetKey("specularPower", &cbDefaultMat.specularPower);
        dc->UpdateSubresource(
            MaterialManager::Instance().defaultMaterialConstantBuffer.Get(),
            0, 0, &cbDefaultMat, 0, 0);

        // 定数バッファに追加
        cbuffers.push_back(MaterialManager::Instance().defaultMaterialConstantBuffer.Get());
    }
}
