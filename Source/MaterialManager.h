#pragma once
#include <Common.h>
#include "MaterialCache.h"

class Material;

class MaterialManager {
private:
    friend class MaterialCache;

    struct CbDefaultMaterial {
        Color diffuseColor;
        float specularPower;
        float DUMMY;
        float DUMMY;
        float DUMMY;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> defaultMaterialConstantBuffer; // デフォルトマテリアル定数バッファ(b3)

    std::map<std::string/* matFilePath */, MaterialCache> caches;

public:
    static MaterialManager& Instance() {
        static MaterialManager instance;
        return instance;
    }

    void Initialize();

    MaterialCache build(Material& material);
    MaterialCache getCache(const std::string& path);

    void setCache(const MaterialCache& cache, const std::string& path);
    void cacheAllMaterials();
    void clearCache(const std::string& path);
    void clearAllCache();
};