#pragma once
#include <Common.h>
#include "Camera.h"
#include <Billboard.h>
#include "GameObject.h"

class Scene;
class Sprite;

class Editor {
private:
    // 管理全般
    std::filesystem::path assetRoot = "Data";
    std::filesystem::path currentDir = assetRoot;
    std::filesystem::path selectedAsset = "";
    bool dirty = false; // アセット編集時(編集できない奴は使わない)
    bool loaded = false; // ロードが必要なプレビューに必要なフラグ
    GameObject* selectedGameObject = nullptr;

    // ファイルorフォルダー追加系
    bool renaming = false;
    std::string renameName = "";
    std::string renamePath = "";
    bool creating = false;
    bool createIsFolder = false;
    std::string createName = "";
    std::string createExt = "";
    std::filesystem::path currentScenePath = "";

    // ギズモ表示
    bool showGizmos = true;

    // カメラ系
    std::unique_ptr<GameObject> g_editorCameraObject;
    Camera* c_saveCamera = nullptr;
    bool camInitialized = false;
    float camYaw = 0.0f;
    float camPitch = 0.0f;
    float camSmoothYaw = 0.0f;
    float camSmoothPitch = 0.0f;
    Vector3 camTargetPos;
    Vector3 camSmoothPos;

    enum class EditorMode
    {
        Edit,
        Play,
        Pause
    } mode = EditorMode::Edit;

    bool GetKey(int key) {
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    void RenderGameObjectTree(Scene* scene, std::vector<std::unique_ptr<GameObject>>& objects);

    void ShowContextMenu(const std::string& label, const std::filesystem::path& path);

    void SaveScene(Scene* scene);

    void SaveSceneAs(Scene* scene);

    void ImportFileWithDialog();

    void RenderDataBrowser();

    void RenderImageDetail(const std::filesystem::path& path);

    void RenderCodeTextDetail(const std::filesystem::path& path);

    void RenderMaterialDetail(const std::filesystem::path& path);

    void RenderAnimationDetail(const std::filesystem::path& path);

    void RenderModelDetail(const std::filesystem::path& path);

    void RenderAssetDetail(const std::filesystem::path& path);

    void RenderDetail();

    void EnterPlay(Scene* scene);

    void EnterPause(Scene* scene);

    void EnterEdit(Scene* scene);

    void EnterResume(Scene* scene);

    void ResetCameraFromTransform(Transform& t);

    void UpdateCamera(Scene* scene);

public:

    static Editor& Instance() {
        static Editor instance = Editor();
        return instance;
    }

    void Initialize(Scene* firstScene);
    void Render(Scene* scene, float fps);

    bool IsEditing() const { return mode == EditorMode::Edit; }
    bool IsPlaying() const { return mode == EditorMode::Play; }
    bool IsPaused() const { return mode == EditorMode::Pause; }
};
