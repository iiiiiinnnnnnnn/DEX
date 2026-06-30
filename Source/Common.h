// Common.h
#pragma once

#include <vector>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <cstdint>
#include <exception> // exception

#include <imgui.h> // imgui
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include "TextEditor.h"
#include <ImGuizmo.h>

#include <d3d11.h> // directx
#include <d3dcompiler.h> // cso compiler
#include <DirectXMath.h> // direct x math
#include <wrl.h> // microsoft wrl
#include <SpriteBatch.h> // directx tool kit
#include <CommonStates.h> // directx tool kit

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define DUMMY CONCAT(__DUMMY__, __LINE__)

// simple math
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;
constexpr float eps = 1e-6f;
constexpr float RAD2DEG = 180.0f / DirectX::XM_PI;
constexpr float DEG2RAD = DirectX::XM_PI / 180.0f;

inline static Color _RGB(Color color) {
    Color notalpha = color;
    notalpha.w = 1.0f;
    return notalpha;
}

// cereals
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

// file info
#include "IconsFontAwesome5.h"
#include <string_view>
#include <commdlg.h>
#include <ShObjIdl_core.h>
#define IsTextFile(ext) IsAny<std::filesystem::path>(ext, ".txt", ".log", ".json", ".markdown", ".md", ".xml", ".ini", ".config", ".conf")
#define IsImageFile(ext) IsAny<std::filesystem::path>(ext, ".dds")
#define IsFontFile(ext) IsAny<std::filesystem::path>(ext, ".ttf", ".otf", ".ttc", ".otc")
#define IsSoundFile(ext) IsAny<std::filesystem::path>(ext, ".wav", ".mp3")
#define IsVideoFile(ext) IsAny<std::filesystem::path>(ext, ".avi", ".mp4")
#define IsPackFile(ext) IsAny<std::filesystem::path>(ext, ".zip", ".tar", ".7z", ".rar")
#define IsBuildFile(ext) IsAny<std::filesystem::path>(ext, ".cso", ".exe", ".spritefont", ".mesh", ".mdl", ".cereal", ".asset")
#define IsModelFile(ext) IsAny<std::filesystem::path>(ext, ".mdl")
#define IsMaterialFile(ext) IsAny<std::filesystem::path>(ext, ".mat")
#define IsCodeFile(ext) IsAny<std::filesystem::path>(ext, ".hlsl", ".hlsli", ".lua")
#define IsSceneFile(ext) IsAny<std::filesystem::path>(ext, ".scene")
#define IsAnimatorFile(ext) IsAny<std::filesystem::path>(ext, ".animator")
#define IsAnimationFile(ext) IsAny<std::filesystem::path>(ext, ".anim")
template<typename T, typename... Ts>
constexpr bool IsAny(const T& value, const Ts&... values) { return ((value == values) || ...); }
constexpr bool IsValidFileName(const std::string& name)
{
    if (name.empty()) return false;
    const std::string invalid = "\\/:*?\"<>|";
    return name.find_first_of(invalid) == std::string::npos;
}
static inline std::string RemoveInvalidFileChars(const std::string& input) {
    std::string result = input;

    // 禁止文字を全部削除（詰める）
    const std::string invalid = "<>:\"/\\|?*";

    result.erase(
        std::remove_if(result.begin(), result.end(),
            [&invalid](char c) { return invalid.find(c) != std::string::npos; }),
        result.end()
    );

    // 追加で末尾のスペースやドットを削除（Windows）
    while (!result.empty() && (result.back() == ' ' || result.back() == '.')) {
        result.pop_back();
    }

    return result;
}
static inline std::optional<std::filesystem::path> SaveFileDialog(
    const std::wstring& defaultName = L"",
    const std::wstring& filter = L"All Files\0*.*\0",
    std::filesystem::path defaultDir = ""
)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
        return std::nullopt;

    IFileSaveDialog* pFileSave = nullptr;
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFileSave));
    if (FAILED(hr))
        return std::nullopt;

    // フィルター設定
    if (!filter.empty())
    {
        std::vector<COMDLG_FILTERSPEC> specs;

        const wchar_t* ptr = filter.c_str();

        while (*ptr)
        {
            const wchar_t* name = ptr;
            ptr += wcslen(ptr) + 1;

            if (*ptr == L'\0') break;

            const wchar_t* spec = ptr;
            ptr += wcslen(ptr) + 1;

            specs.push_back({ name, spec });
        }

        if (!specs.empty())
        {
            pFileSave->SetFileTypes((UINT)specs.size(), specs.data());
            pFileSave->SetFileTypeIndex(1); // デフォルト選択
        }
    }

    // デフォルトファイル名
    if (!defaultName.empty())
    {
        pFileSave->SetFileName(defaultName.c_str());
    }

    if(!defaultDir.empty())
    {
        // フォルダが無ければ作成
        std::filesystem::create_directories(defaultDir);

        IShellItem* pFolder = nullptr;
        HRESULT hrFolder = SHCreateItemFromParsingName(
            defaultDir.wstring().c_str(),
            nullptr,
            IID_PPV_ARGS(&pFolder)
        );

        if (SUCCEEDED(hrFolder) && pFolder)
        {
            pFileSave->SetFolder(pFolder);
            pFolder->Release();
        }
    }

    // 上書き確認を有効化
    DWORD options;
    pFileSave->GetOptions(&options);
    pFileSave->SetOptions(options | FOS_OVERWRITEPROMPT);

    // ダイアログ表示
    hr = pFileSave->Show(NULL);
    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;
        hr = pFileSave->GetResult(&pItem);
        if (SUCCEEDED(hr) && pItem)
        {
            PWSTR pszFilePath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
            if (SUCCEEDED(hr) && pszFilePath)
            {
                std::filesystem::path path(pszFilePath);
                CoTaskMemFree(pszFilePath);
                pItem->Release();
                pFileSave->Release();
                CoUninitialize();
                return path;
            }
            pItem->Release();
        }
    }

    pFileSave->Release();
    CoUninitialize();
    return std::nullopt;
}
static inline std::optional<std::filesystem::path> OpenFileDialog(
    const std::wstring& filter = L"All Files\0*.*\0"
)
{
    // COM 初期化
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
        return std::nullopt;

    IFileOpenDialog* pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFileOpen));
    if (FAILED(hr))
        return std::nullopt;

    // フィルター設定
    if (!filter.empty())
    {
        COMDLG_FILTERSPEC specs[] = { { L"All Files", L"*.*" } };
        pFileOpen->SetFileTypes(1, specs);
    }

    // ダイアログ表示
    hr = pFileOpen->Show(NULL);
    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;
        hr = pFileOpen->GetResult(&pItem);
        if (SUCCEEDED(hr) && pItem)
        {
            PWSTR pszFilePath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
            if (SUCCEEDED(hr) && pszFilePath)
            {
                std::filesystem::path path(pszFilePath);
                CoTaskMemFree(pszFilePath);
                pItem->Release();
                pFileOpen->Release();
                CoUninitialize();
                return path; // 絶対パス
            }
            pItem->Release();
        }
    }

    pFileOpen->Release();
    CoUninitialize();
    return std::nullopt;
}

// for move file to trash in windows os
#include <windows.h>
#include <shellapi.h>
static bool MoveToRecycleBin(const std::filesystem::path& path)
{
    std::wstring wpath = path.wstring();
    wpath.push_back(L'\0'); // SHFileOperationは二重NULL終端が必要

    SHFILEOPSTRUCTW op = {};
    op.wFunc = FO_DELETE;
    op.pFrom = wpath.c_str();
    op.fFlags =
        FOF_ALLOWUNDO |      // ← ゴミ箱に送る
        FOF_NOCONFIRMATION | // 確認ダイアログなし
        FOF_NOERRORUI |      // エラーUIなし
        FOF_SILENT;

    return SHFileOperationW(&op) == 0;
}

// json
#include <nlohmann/json.hpp>
using namespace nlohmann;

// user headers
#include "Misc.h"
#include "GpuResourceUtils.h"
#include "HighResolutionTimer.h"
#include "TransformUtils.h"
#include "DefinedParameters.h"

// hash
static inline std::string hashCode(const std::string& str) {
    std::ostringstream oss;
    oss << std::hex << std::hash<std::string>{}(str);
    return oss.str();
}

// logger
#include "Logger.h"

// xtex
#include <DirectXTex.h>
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>