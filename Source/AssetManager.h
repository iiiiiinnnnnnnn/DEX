#pragma once
#include <Common.h>
#include <MaterialManager.h>

class AssetManager {
public:
    static AssetManager& Instance() {
        static AssetManager instance;
        return instance;
    }

    void Initialize()
    {
        if (!std::filesystem::exists("Cache"))
            std::filesystem::create_directory("Cache");

        MaterialManager::Instance().Initialize();
    }

    std::vector<std::filesystem::path> ScanAssets(std::string extension) {
        static std::vector<std::filesystem::path> ddsAssets;
        ddsAssets.clear();
        const char DataFolder[] = "Data";
        for (auto& p : std::filesystem::recursive_directory_iterator(DataFolder))
        {
            if (!p.is_regular_file()) continue;

            auto ext = p.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == extension.c_str())
                ddsAssets.push_back(p.path());
        }

        return ddsAssets;
    }
};