#pragma once

#include <Common.h>

// デフォルトマテリアル定数バッファはManagerで保持
class MaterialCache {
private:
    friend class MaterialManager;

    struct ReflectedCBVar { UINT offset; UINT size; };
    std::map<std::string, ReflectedCBVar> materialCBVars;
    std::vector<uint8_t> materialCpuBuffer;
    UINT materialCBSize;

    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> materialConstantBuffer; // カスタムマテリアル定数バッファ(b3)

public:
    DefinedParameters reflectedParams;
	bool isCustomShader = false;

    void setPixelShader(ID3D11PixelShader* ps) { pixelShader = ps; }
    ID3D11PixelShader* getPixelShader() { return pixelShader.Get(); }

    void Apply(std::vector<ID3D11Buffer*>& cbuffers);

    void updateCBuffers(std::vector<ID3D11Buffer*>& cbuffers);
};