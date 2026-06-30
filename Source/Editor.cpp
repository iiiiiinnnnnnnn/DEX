#include "Editor.h"
#include "Scene.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <Graphics.h>
#include <AssimpImporter.h>

#include "Model.h"
#include <Material.h>
#include <AssetManager.h>
#include <ImGuiRenderer.h>

float camDistance = 8.f;
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

void Editor::Initialize(Scene* firstScene)
{
    // エディタ用カメラ生成
    g_editorCameraObject = std::make_unique<GameObject>("EditorCamera");
    c_saveCamera = g_editorCameraObject->AddComponent<Camera>();

    // 最初にカメラ情報をセーブ
	auto mc = firstScene->GetMainCamera();
    if (mc)
        mc->CopyTo(c_saveCamera);
}

void Editor::EnterPlay(Scene* scene)
{
    mode = EditorMode::Play;

	// カメラ情報をロード
    auto mc = scene->GetMainCamera();
    if (mc)
        c_saveCamera->CopyTo(mc);
}

void Editor::EnterPause(Scene* scene)
{
    mode = EditorMode::Pause;

	// カメラ情報をセーブ
    auto mc = scene->GetMainCamera();
    if (mc) {
        mc->CopyTo(c_saveCamera);
        ResetCameraFromTransform(mc->owner->transform);
    }
}

void Editor::EnterEdit(Scene* scene)
{
    mode = EditorMode::Edit;

    // カメラ情報をセーブ
    auto mc = scene->GetMainCamera();
    if (mc) {
        mc->CopyTo(c_saveCamera);
        ResetCameraFromTransform(mc->owner->transform);
    }
}

void Editor::EnterResume(Scene* scene)
{
    mode = EditorMode::Play;

    // カメラ情報をロード
    auto mc = scene->GetMainCamera();
    if (mc)
        c_saveCamera->CopyTo(mc);
}

void Editor::ResetCameraFromTransform(Transform& t)
{
    Vector3 f = Vector3::Transform(Vector3::Forward, t.rotation);
    f.Normalize();

    camYaw = atan2f(-f.x, -f.z);
    camPitch = asinf(f.y);

    camSmoothYaw = camYaw;
    camSmoothPitch = camPitch;

    camTargetPos = t.position;
    camSmoothPos = t.position;
}

void Editor::UpdateCamera(Scene* scene)
{
    auto mc = scene->GetMainCamera();
    if (!mc) return;

    const ImGuiIO& io = ImGui::GetIO();
    auto& t = mc->owner->transform;

    // =======================
    // Editorカメラの“本体状態”
    // =======================

    // =======================
    // 初期化（1回だけ）
    // =======================
    if (!camInitialized)
    {
        camTargetPos = t.position;
        camSmoothPos = t.position;

        Vector3 f = Vector3::Transform(Vector3::Forward, t.rotation);
        f.Normalize();

        camYaw = atan2f(-f.x, -f.z);
        camPitch = asinf(f.y);

        camSmoothYaw = camYaw;
        camSmoothPitch = camPitch;

        camInitialized = true;
    }

    bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    // =======================
    // 入力：回転（target更新）
    // =======================
    if (isMouseDown)
    {
        float sensitivity = 0.002f;

        camYaw -= io.MouseDelta.x * sensitivity;
        camPitch -= io.MouseDelta.y * sensitivity;

        float limit = DirectX::XM_PIDIV2 - 0.01f;
        camPitch = std::clamp(camPitch, -limit, limit);
    }

    // =======================
    // 回転スムージング
    // =======================
    float rotLerp = 1.0f - std::exp(-15.0f * io.DeltaTime);

    float diff = camYaw - camSmoothYaw;
    if (diff > DirectX::XM_PI)  diff -= DirectX::XM_2PI;
    if (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

    camSmoothYaw += diff * rotLerp;
    camSmoothPitch = std::lerp(camSmoothPitch, camPitch, rotLerp);

    Quaternion rot = Quaternion::CreateFromYawPitchRoll(camSmoothYaw, camSmoothPitch, 0.0f);

    // =======================
    // 入力：移動（target更新）
    // =======================
    if (isMouseDown)
    {
        Vector3 forward = Vector3::Transform(Vector3::Forward, rot);

        Vector3 right;
        Vector3::Up.Cross(forward, right);
        right.Normalize();

        float moveSpeed = GetKey(VK_SHIFT) ? 15.0f : 5.0f;
        float dt = io.DeltaTime;

        if (GetKey('W')) camTargetPos += forward * moveSpeed * dt;
        if (GetKey('S')) camTargetPos -= forward * moveSpeed * dt;
        if (GetKey('A')) camTargetPos += right * moveSpeed * dt;
        if (GetKey('D')) camTargetPos -= right * moveSpeed * dt;
        if (GetKey('E')) camTargetPos += Vector3::Up * moveSpeed * dt;
        if (GetKey('Q')) camTargetPos -= Vector3::Up * moveSpeed * dt;
    }

    // =======================
    // ホイール移動
    // =======================
    if (io.MouseWheel != 0.0f && !ImGui::IsAnyItemHovered())
    {
        Vector3 forward = Vector3::Transform(Vector3::Forward, rot);
        camTargetPos += forward * io.MouseWheel * 5.0f;
    }

    // =======================
    // 位置スムージング
    // =======================
    float posLerp = 1.0f - std::exp(-10.0f * io.DeltaTime);
    camSmoothPos = Vector3::Lerp(camSmoothPos, camTargetPos, posLerp);

    // =======================
    // 最終反映
    // =======================
    t.rotation = rot;
    t.position = camSmoothPos;
}

void Editor::RenderGameObjectTree(Scene* scene, std::vector<std::unique_ptr<GameObject>>& objects)
{
    int index = 0;

    for (auto& obj : objects)
    {
        ImGui::PushID(obj.get());

        // -------------------------
        // 並び替え DropZone
        // -------------------------
        /*if (ImGui::IsDragDropActive())
        {
            ImGui::InvisibleButton("dropzone", ImVec2(-FLT_MIN, 10));

            if (ImGui::IsItemHovered())
            {
                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();

                draw->AddRectFilled(
                    min,
                    max,
                    IM_COL32(100, 150, 255, 120)
                );
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("DND_GAMEOBJECT"))
                {
                    GameObject* dragged =
                        *(GameObject**)payload->Data;

                    if (dragged != obj.get())
                    {
                        auto& list = obj->parent ?
                            obj->parent->children :
                            scene->GetRootObjects();

                        auto it = std::find(list.begin(), list.end(), dragged);
                        if (it != list.end())
                            list.erase(it);

                        list.insert(list.begin() + index, dragged);
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }*/

        // -------------------------
        // TreeNode
        // -------------------------
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

        if (obj->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (selectedGameObject == obj.get())
            flags |= ImGuiTreeNodeFlags_Selected;

        bool is_open = ImGui::TreeNodeEx(
            obj.get(),
            flags,
            ICON_FA_CUBE " %s",
            obj->name.c_str()
        );

        // 選択
        if (ImGui::IsItemClicked())
            selectedGameObject = obj.get();

        // -------------------------
        // Drag開始
        // -------------------------
        /*if (ImGui::BeginDragDropSource())
        {
            GameObject* payload = obj.get();

            ImGui::SetDragDropPayload(
                "DND_GAMEOBJECT",
                &payload,
                sizeof(GameObject*)
            );

            ImGui::Text("%s", obj->name.c_str());

            ImGui::EndDragDropSource();
        }*/

        // -------------------------
        // 子オブジェクトDrop
        // -------------------------
        /*if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("DND_GAMEOBJECT"))
            {
                GameObject* dragged =
                    *(GameObject**)payload->Data;

                if (dragged != obj.get())
                {
                    dragged->SetParent(scene, obj.get(), true);
                }
            }

            ImGui::EndDragDropTarget();
        }*/

        // -------------------------
        // 子描画
        // -------------------------
        if (is_open)
        {
            if (!obj->children.empty())
            {
                RenderGameObjectTree(scene, obj->children);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();

        index++;
    }

    // -----------------------------
    // 最下部 DropZone
    // -----------------------------
    /*if (ImGui::IsDragDropActive())
    {
        ImGui::InvisibleButton("bottom_dropzone", ImVec2(-FLT_MIN, 10));

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("DND_GAMEOBJECT"))
            {
                GameObject* dragged =
                    *(GameObject**)payload->Data;

                auto& list = scene->GetRootObjects();

                auto it = std::find(list.begin(), list.end(), dragged);
                if (it != list.end())
                    list.erase(it);

                list.push_back(dragged);
            }

            ImGui::EndDragDropTarget();
        }
    }*/
}

void Editor::ShowContextMenu(const std::string& label, const std::filesystem::path& path) {

    ImGui::TextDisabled((std::string((const char*)u8"編集: ") + label).c_str());
    ImGui::Separator();
    if (ImGui::MenuItem((const char*)u8"複製")) {

        auto parent = path.parent_path();
        auto stem = path.stem().string();
        auto ext = path.extension().string();

        int counter = 1;
        std::filesystem::path newPath;
        do {
            newPath = parent / (stem + "_" + std::to_string(counter) + ext);
            counter++;
        } while (std::filesystem::exists(newPath));

        std::filesystem::copy(path, newPath);
    }
    if (ImGui::MenuItem((const char*)u8"名前を変更")) {
        renaming = true;
        renameName = path.filename().string();
        renamePath = path.string();
    }
    if (ImGui::MenuItem((const char*)u8"削除"))
    {
        MoveToRecycleBin(path); // ゴミ箱に移動
        if (selectedAsset == path)
            selectedAsset.clear();
    }
    ImGui::EndPopup();
}

void Editor::SaveScene(Scene* scene)
{
    auto savePath = std::filesystem::path("Data/Scene/") / currentScenePath.filename();
    savePath = savePath.replace_extension(".scene");
    scene->Save(savePath);

    currentScenePath = savePath.filename().stem();
}

void Editor::SaveSceneAs(Scene* scene)
{
    std::wstring defaultFileName = L"untitled";
    int index = 0;
    while (std::filesystem::exists(
        defaultFileName +
        (index == 0 ? L"" : L"(" + std::to_wstring(index) + L")")
        + L".scene"))
    { index++; }

    auto path = SaveFileDialog(
        defaultFileName,
        L"Scene File (*.scene)\0*.scene\0",
        std::filesystem::current_path() / "Data\\Scene"
    );

    if (path) {
        auto savePath = path.value();
        savePath = savePath.replace_extension(".scene");
        scene->Save(savePath);

        currentScenePath = savePath.filename().stem();
    }
}

void Editor::ImportFileWithDialog()
{
    auto path = OpenFileDialog();
    if (path)
    {
        auto pathS = path->string();
        auto name = path->filename().stem().string();
        auto ext = path->filename().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (ext == ".fbx" || ext == ".gltf" || ext == ".glb")
        {
            // 汎用モデルファイルの読み込み
            AssimpImporter importer(pathS.c_str());

            std::vector<Model::Mesh> meshes;
            std::vector<Material> materials;
            std::vector<Model::Node> nodes;
            std::vector<Model::Animation> animations;

            if (!std::filesystem::exists(currentDir / name))
                std::filesystem::create_directory(currentDir / name);

            // マテリアルデータ読み取り
            importer.LoadMaterials(currentDir / name, materials);

            // ノードデータ読み取り
            importer.LoadNodes(nodes);

            // メッシュデータ読み取り
            importer.LoadMeshes(meshes, nodes);

            // アニメーションデータ読み取り
            importer.LoadAnimations(animations, nodes);

            std::vector<std::string> materialSlotNames;
            std::vector<std::string> materialPathes;

            // シリアライズ：マテリアル
            for (auto& mat : materials)
            {
                auto matPath = currentDir / name / (RemoveInvalidFileChars(mat.name) + ".mat");
                materialPathes.push_back(matPath.string()); // マテリアルのパスだけ保存する
                materialSlotNames.push_back(RemoveInvalidFileChars(mat.name));
                mat.SaveToJson(matPath);
            }

            // シリアライズ：本体
            std::ofstream mdlstream(currentDir / name / (name + ".mdl"), std::ios::binary);
            if (mdlstream.is_open())
            {
                cereal::BinaryOutputArchive archive(mdlstream);
                archive(CEREAL_NVP(nodes), CEREAL_NVP(meshes), CEREAL_NVP(materialSlotNames), CEREAL_NVP(materialPathes));
            }

            // シリアライズ：アニメーション
            for (auto& anim : animations)
            {
                std::ofstream animstream(currentDir / name / (RemoveInvalidFileChars(anim.name) + ".anim"), std::ios::binary);
                if (animstream.is_open())
                {
                    cereal::BinaryOutputArchive archive(animstream);
                    archive(CEREAL_NVP(anim));
                }
            }
        }
        else if (ext == ".bmp" ||
            ext == ".png" ||
            ext == ".jpg" || ext == ".jpeg" ||
            ext == ".tif" || ext == ".tiff" ||
            ext == ".gif" ||
            ext == ".hdp" || // JPEG XR
            ext == ".jxr" ||
            ext == ".wdp" ||
            ext == ".ico")
        {
            GpuResourceUtils::ConvertToDDS(path->wstring().c_str(), (currentDir / (name + ".dds")).c_str());
        }
        else if (ext == ".dds")
        {
			std::filesystem::copy(path.value(), currentDir / path->filename());
        }
    }
}

void Editor::RenderDataBrowser()
{
    if (ImGui::Button(ICON_FA_PLUS))
    {
        ImGui::OpenPopup("CreatePopup");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_DOWNLOAD))
    {
        ImportFileWithDialog();
    }

    if (ImGui::BeginPopup("CreatePopup"))
    {
        ImGui::TextDisabled((const char*)u8"新しく作る");
        ImGui::Separator();

        if (ImGui::MenuItem((std::string(ICON_FA_FOLDER_PLUS) + (const char*)u8" フォルダー").c_str()))
        {
            creating = true;
            createIsFolder = true;
            createName = "";
            createExt = "";
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();

        if (ImGui::MenuItem((std::string(ICON_FA_PAINT_BRUSH) + (const char*)u8" マテリアル").c_str()))
        {
            creating = true;
            createIsFolder = false;
            createName = "";
            createExt = ".mat";
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem((std::string(ICON_FA_FILE_CODE) + (const char*)u8" HLSLヘッダー").c_str()))
        {
            creating = true;
            createIsFolder = false;
            createName = "";
            createExt = ".hlsli";
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::Text("%s", currentDir.string().c_str());
    ImGui::Separator();

    if (currentDir != assetRoot)
    {
        if (ImGui::Selectable(".."))
        {
            currentDir = currentDir.parent_path();
            return;
        }
    }

    std::vector<std::filesystem::directory_entry> dirs;
    std::vector<std::filesystem::directory_entry> files;

    for (auto& entry : std::filesystem::directory_iterator(currentDir))
    {
        if (entry.is_directory())
            dirs.push_back(entry);
        else
            files.push_back(entry);
    }

    if (creating)
    {
        ImGui::PushID("CreateAsset");

        ImGui::InputText(
            "",
            &createName,
            ImGuiInputTextFlags_EnterReturnsTrue
        );

        ImGui::SetKeyboardFocusHere(-1);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            if (IsValidFileName(createName))
            {
                std::filesystem::path newPath = currentDir / createName.append(createExt);

                if (createIsFolder) {
                    std::filesystem::create_directory(newPath);
                }
                else if (createExt == ".mat")
                {
                    Material mat;
                    mat.SaveToJson(newPath);
                }
                else {
                    // ただの空ファイル
                    std::ofstream(newPath.string()).close();
                }
            }

            creating = false;
        }

        ImGui::PopID();
        ImGui::Separator();
    }
    else if (renaming)
    {
        ImGui::PushID("RenameAsset");

        ImGui::InputText(
            "",
            &renameName,
            ImGuiInputTextFlags_EnterReturnsTrue
        );

        ImGui::SetKeyboardFocusHere(-1);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            if (IsValidFileName(renameName))
            {
                std::filesystem::path newPath = currentDir / renameName;
                std::filesystem::rename(renamePath, newPath);
            }

            renaming = false;
        }
        if (ImGui::IsItemDeactivated())
        {
            renaming = false;
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    // --- ディレクトリ ---
    for (auto& entry : dirs)
    {
        const std::filesystem::path& path = entry.path();
        std::string label = ICON_FA_FOLDER " " + path.filename().string();

        bool selected = (selectedAsset == path);

        if (ImGui::Selectable(label.c_str(), selected)) {
            selectedAsset = path;
            currentDir = path;
        }

        if (ImGui::BeginPopupContextItem())
        {
            ShowContextMenu(label, path);
        }
    }

    // --- ファイル ---
    for (auto& entry : files)
    {
        const std::filesystem::path& path = entry.path();
        std::string icon = ICON_FA_FILE " ";
        std::string ext = path.extension().string();

        // lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (IsImageFile(ext))           icon = ICON_FA_FILE_IMAGE " ";
        else if (IsTextFile(ext))       icon = ICON_FA_FILE_CODE " ";
        else if (IsFontFile(ext))       icon = ICON_FA_FONT " ";
        else if (IsSoundFile(ext))      icon = ICON_FA_MUSIC " ";
        else if (IsVideoFile(ext))      icon = ICON_FA_FILE_VIDEO " ";
        else if (IsPackFile(ext))       icon = ICON_FA_PARKING " ";
        else if (IsBuildFile(ext))      icon = ICON_FA_CUBE " ";
        else if (IsModelFile(ext))      icon = ICON_FA_SHAPES " ";
        else if (IsMaterialFile(ext))   icon = ICON_FA_PAINT_BRUSH " ";
        else if (IsCodeFile(ext))       icon = ICON_FA_FILE_CODE " ";
        else if (IsSceneFile(ext))      icon = ICON_FA_CUBES " ";
        else if (IsAnimatorFile(ext))   icon = ICON_FA_SITEMAP " ";
        else if (IsAnimationFile(ext))  icon = ICON_FA_RUNNING " ";

        std::string label = icon + path.filename().string();

        bool selected = (selectedAsset == path);

        if (ImGui::Selectable(label.c_str(), selected)) {
            dirty = false;
            loaded = false;
            selectedAsset = path;
            selectedGameObject = nullptr;
        }

        if (ImGui::BeginPopupContextItem())
        {
            ShowContextMenu(label, path);
        }
    }
}

void Editor::RenderImageDetail(const std::filesystem::path& path)
{
    static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;

    ImGui::Separator();

    if (!loaded)
    {
        HRESULT hr = DirectX::CreateDDSTextureFromFile(
            Graphics::Instance().GetDevice(),
            path.wstring().c_str(),
            nullptr,
            shaderResourceView.GetAddressOf()
        );
        if (hr != S_OK) {
            ImGui::TextColored({ 1, 0, 0, 1 }, (const char*)u8"読み込みエラー");
            return;
        }
        loaded = true;
    }

    ImGui::Image(
        (ImTextureID)shaderResourceView.Get(),
        ImGui::GetContentRegionAvail()
    );
}

void Editor::RenderCodeTextDetail(const std::filesystem::path& path)
{
    static TextEditor editor;

    if (dirty)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button((const char*)u8"保存"))
        {
            // --- 保存処理 ---
            std::ofstream ofs(path, std::ios::out | std::ios::binary);
            if (ofs)
            {
                ofs << editor.GetText();
                ofs.close();
                dirty = false;
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "*modified");
    }

    ImGui::Separator();

    editor.Render((const char*)u8"コードエディタ", ImGui::GetContentRegionAvail());
    if (editor.IsTextChanged())
        dirty = true;
}

void Editor::RenderMaterialDetail(const std::filesystem::path& path)
{
    static TextEditor editor;
    static Material mat;
    static int custom = 0;
    static bool buildSuccess = false;
    static std::vector<std::filesystem::path> ddsList;
    auto& matManager = MaterialManager::Instance();

    if (!loaded)
    {
        if (!mat.LoadFromJson(path)) {
            ImGui::TextColored({ 1, 0, 0, 1 }, (const char*)u8"読み込みエラー");
            return;
        }
        editor.SetText(mat.GetPixelShaderCode());
        custom = mat.IsCustomShader();
        buildSuccess = mat.CanBuild();
        ddsList = AssetManager::Instance().ScanAssets(".dds");
        loaded = true;
    }

    // キャッシュ取得
    auto cache = matManager.getCache(path.string());

    ImGui::TextDisabled((const char*)u8"パラメーター");
    ImGui::TreePush();

    for (auto& pair : cache.reflectedParams.getFloats())
    {
        float value = pair.second; // コピー

        if (ImGui::DragFloat(
            (std::string(ICON_FA_PASTE " ") + pair.first).c_str(),
            &value))
        {
            mat.saveParams.setKey(pair.first, value);
            mat.SaveToJson(path);
            matManager.clearCache(path.string());
        }
    }

    for (auto& pair : cache.reflectedParams.getColors())
    {
        DirectX::XMFLOAT4 value = pair.second;

        if (ImGui::ColorEdit4(
            (std::string(ICON_FA_PALETTE " ") + pair.first).c_str(),
            (float*)&value))
        {
            mat.saveParams.setKey(pair.first, value);
            mat.SaveToJson(path);
            matManager.clearCache(path.string());
        }
    }

    for (auto& pair : cache.reflectedParams.getTextures())
    {
        bool changed = false;

        ImGui::Text("%s %s", ICON_FA_FILE_IMAGE, pair.first.c_str());
        ImGui::SameLine();

        const char* preview = pair.second.path.empty()
            ? "<none>"
            : pair.second.path.c_str();

        if (ImGui::BeginCombo(("##" + pair.first).c_str(), preview))
        {
            if (ImGui::Selectable("<none>", pair.second.path.empty()))
            {
                if (!pair.second.path.empty())
                {
                    pair.second.path.clear();
                    changed = true;
                }
            }

            for (auto& dds : ddsList)
            {
                std::string rel = dds.string();
                bool selected = (rel == pair.second.path);

                if (ImGui::Selectable(rel.c_str(), selected))
                {
                    if (pair.second.path != rel)
                    {
                        pair.second.path = rel;
                        changed = true;
                    }
                }

                static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                static std::string cachedPath;

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(rel.c_str());
                    ImGui::Separator();

                    if (cachedPath != rel)
                    {
                        srv.Reset();
                        if (SUCCEEDED(DirectX::CreateDDSTextureFromFile(
                            Graphics::Instance().GetDevice(),
                            std::filesystem::path(rel).wstring().c_str(),
                            nullptr,
                            srv.GetAddressOf())))
                        {
                            cachedPath = rel;
                        }
                    }

                    if (srv)
                    {
                        ImGui::Image(
                            (ImTextureID)srv.Get(),
                            ImVec2(128, 128)
                        );
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "SRV NULL");
                    }

                    ImGui::EndTooltip();
                }

                if (ImGui::IsItemClicked())
                {
                    pair.second.path = rel;
                }
            }

            ImGui::EndCombo();
        }

        if (changed)
        {
            mat.saveParams.setKey(pair.first, pair.second);
            mat.SaveToJson(path);
            matManager.clearCache(path.string());
        }
    }

    ImGui::TreePop();

    ImGui::TextDisabled((const char*)u8"シェーダー");

    ImGui::SameLine();

    if (ImGui::RadioButton((const char*)u8"デフォルト", (int*)&custom, 0))
    {
        mat.isCustomShader = false;
        mat.SaveToJson(path);
        buildSuccess = mat.CanBuild();
    }

    ImGui::SameLine();

    if (ImGui::RadioButton((const char*)u8"カスタム", (int*)&custom, 1))
    {
        mat.isCustomShader = true;
        mat.SaveToJson(path);
        buildSuccess = mat.CanBuild();
    }

    if (custom)
    {
        ImGui::TreePush();

        if (dirty)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button((const char*)u8"保存"))
            {
                mat.SetPixelShaderCode(editor.GetText());
                mat.isCustomShader = true;
                mat.SaveToJson(path);
                buildSuccess = mat.CanBuild();
                dirty = false;
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), "*modified");
        }
        else
        {
            if (buildSuccess)
                ImGui::TextColored(ImVec4(0.6f, 1, 0.6f, 1), (const char*)u8"ビルド成功");
            else
                ImGui::TextColored(ImVec4(1, 0.6f, 0.6f, 1), (const char*)u8"*ビルドに失敗しました");
        }

        ImGui::Separator();

        editor.Render((const char*)u8"コードエディタ", ImGui::GetContentRegionAvail());
        if (editor.IsTextChanged())
            dirty = true;

        ImGui::TreePop();
    }
}

void Editor::RenderAnimationDetail(const std::filesystem::path& path)
{
    using Anim = Model::Animation;

    static Anim anim;
    static int selectedNode = -1;

    if (!loaded)
    {
        anim = Model::Animation::LoadFromFilePath(path.string());
        loaded = true;
    }

    // =========================
    // 基本情報
    // =========================
    ImGui::Text("Name: %s", anim.name.c_str());
    ImGui::Text("Length: %.3f sec", anim.secondsLength);
    ImGui::Separator();

    // =========================
    // Node一覧
    // =========================
    ImGui::BeginChild("nodes", ImVec2(200, 0), true);

    for (int i = 0; i < anim.nodeAnims.size(); i++)
    {
        std::string label = "Node " + std::to_string(i);

        if (ImGui::Selectable(label.c_str(), selectedNode == i))
            selectedNode = i;
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // =========================
    // 詳細
    // =========================
    ImGui::BeginChild("detail", ImVec2(0, 0), true);

    if (selectedNode >= 0)
    {
        auto& node = anim.nodeAnims[selectedNode];

        // =========================
        // 再生ヘッド（重要）
        // =========================
        static float currentTime = 0.f;
        ImGui::SliderFloat("Time", &currentTime, 0.0f, anim.secondsLength);

        ImGui::Separator();

        // =========================
        // Position
        // =========================
        if (ImGui::TreeNode("Position"))
        {
            static int removeIndex = -1;

            for (int i = 0; i < node.positionKeyframes.size(); i++)
            {
                auto& k = node.positionKeyframes[i];

                ImGui::PushID(i);

                ImGui::DragFloat("sec", &k.seconds, 0.01f);
                ImGui::DragFloat3("value", (float*)&k.value, 0.01f);

                if (ImGui::Button("Delete"))
                    removeIndex = i;

                ImGui::Separator();
                ImGui::PopID();
            }

            if (removeIndex >= 0)
            {
                node.positionKeyframes.erase(
                    node.positionKeyframes.begin() + removeIndex);
                removeIndex = -1;
            }

            if (ImGui::Button("Add Key"))
            {
                node.positionKeyframes.push_back({ currentTime, {0,0,0} });
            }

            // ===== グラフ =====
            std::vector<float> graph;
            for (auto& k : node.positionKeyframes)
                graph.push_back(k.value.x);

            if (!graph.empty())
                ImGui::PlotLines("PosX", graph.data(), (int)graph.size());

            ImGui::TreePop();
        }

        // =========================
        // Rotation
        // =========================
        if (ImGui::TreeNode("Rotation"))
        {
            for (int i = 0; i < node.rotationKeyframes.size(); i++)
            {
                auto& k = node.rotationKeyframes[i];

                ImGui::PushID(i);

                ImGui::DragFloat("sec", &k.seconds, 0.01f);

                Vector3 euler = k.value.ToEuler(); // 必須
                if (ImGui::DragFloat3("rot", (float*)&euler, 0.5f))
                {
                    k.value = Quaternion::CreateFromYawPitchRoll(
                        euler.y, euler.x, euler.z);
                }

                ImGui::Separator();
                ImGui::PopID();
            }

            // グラフ（Y回転）
            std::vector<float> graph;
            for (auto& k : node.rotationKeyframes)
            {
                auto e = k.value.ToEuler();
                graph.push_back(e.y);
            }

            if (!graph.empty())
                ImGui::PlotLines("RotY", graph.data(), (int)graph.size());

            ImGui::TreePop();
        }

        // =========================
        // Scale
        // =========================
        if (ImGui::TreeNode("Scale"))
        {
            for (int i = 0; i < node.scaleKeyframes.size(); i++)
            {
                auto& k = node.scaleKeyframes[i];

                ImGui::PushID(i);

                ImGui::DragFloat("sec", &k.seconds, 0.01f);
                ImGui::DragFloat3("value", (float*)&k.value, 0.01f);

                ImGui::Separator();
                ImGui::PopID();
            }

            std::vector<float> graph;
            for (auto& k : node.scaleKeyframes)
                graph.push_back(k.value.x);

            if (!graph.empty())
                ImGui::PlotLines("ScaleX", graph.data(), (int)graph.size());

            ImGui::TreePop();
        }

        ImGui::Separator();

        // =========================
        // 保存
        // =========================
        if (ImGui::Button("Save"))
        {
            std::ofstream os(path, std::ios::binary);
            cereal::BinaryOutputArchive ar(os);
            ar(anim);
        }
    }

    ImGui::EndChild();
}

void Editor::RenderModelDetail(const std::filesystem::path& path)
{
    using Node = Model::Node;
    using Mesh = Model::Mesh;

    static std::vector<std::filesystem::path> ddsList;
    static std::vector<Model::Node> nodes;
    static std::vector<Model::Mesh> meshes;
    static std::vector<std::string> materialSlotNames;
    static std::vector<std::string> materialPathes;
    if (!loaded)
    {
        nodes.clear();
        meshes.clear();

        // デシリアライズする
        std::ifstream istream(path, std::ios::binary);
        if (istream.is_open())
        {
            cereal::BinaryInputArchive archive(istream);
            try {
                archive(
                    CEREAL_NVP(nodes),
                    CEREAL_NVP(meshes),
                    CEREAL_NVP(materialSlotNames),
                    CEREAL_NVP(materialPathes)
                );
            }
            catch (...)
            {
                _ASSERT_EXPR_A(false, "Model deserialize failed.");
            }
        }
        else
        {
            _ASSERT_EXPR_A(false, "Model File not found.");
        }
        ddsList = AssetManager::Instance().ScanAssets(".mat");
        loaded = true;
        dirty = false;
    }

    ImGui::TextDisabled((const char*)u8"マテリアルスロット");
    ImGui::TreePush();
    for (int i = 0; i < materialPathes.size(); i++)
    {
        ImGui::Text("%s [%d] %s", ICON_FA_PAINT_BRUSH, i, materialSlotNames[i].c_str());
        ImGui::SameLine();

        const char* preview =
            materialPathes[i].empty() ? "<none>" : materialPathes[i].c_str();

        if (ImGui::BeginCombo(("##" + materialSlotNames[i]).c_str(), preview))
        {
            if (ImGui::Selectable("<none>", materialPathes[i].empty()))
            {
                materialPathes[i].clear();
                dirty = true;
            }

            for (auto& dds : ddsList)
            {
                std::string rel = dds.string();
                bool selected = (rel == materialPathes[i]);

                if (ImGui::Selectable(rel.c_str(), selected))
                {
                    materialPathes[i] = rel;
                    dirty = true;
                }
            }
            ImGui::EndCombo();
        }
    }
    ImGui::TreePop();

    if (dirty)
    {
        std::ofstream mdlstream(path, std::ios::binary);
        if (mdlstream.is_open())
        {
            cereal::BinaryOutputArchive archive(mdlstream);
            archive(CEREAL_NVP(nodes), CEREAL_NVP(meshes), CEREAL_NVP(materialSlotNames), CEREAL_NVP(materialPathes));
        }
    }
}

void Editor::RenderAssetDetail(const std::filesystem::path& path)
{
    ImGui::TextDisabled((const char*)u8"%s", path.filename().string().c_str());

    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
    {
        constexpr float KB = 1024.0f;
        constexpr float MB = 1024.0f * 1024.0f;
        constexpr float GB = 1024.0f * 1024.0f * 1024.0f;

        char buf[64];
        auto bytes = std::filesystem::file_size(path);

        if (bytes >= GB)        snprintf(buf, sizeof(buf), "%.2f GB", bytes / GB);
        else if (bytes >= MB)   snprintf(buf, sizeof(buf), "%.2f MB", bytes / MB);
        else if (bytes >= KB)   snprintf(buf, sizeof(buf), "%.2f KB", bytes / KB);
        else                    snprintf(buf, sizeof(buf), "%llu B", bytes);

        ImGui::TextDisabled((const char*)u8"サイズ: %s", buf);
    }

    /*std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });*/

    std::string ext = path.extension().string();

    if (ext.empty()) {
        std::string filename = path.filename().string();
        if (!filename.empty() && filename[0] == '.') {
            ext = filename;
        }
    }

    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    ImGui::Separator();

    // 種類別
    if (IsImageFile(ext))                           RenderImageDetail(path);
    else if (IsCodeFile(ext) || IsTextFile(ext))    RenderCodeTextDetail(path);
    else if (IsMaterialFile(ext))                   RenderMaterialDetail(path);
    else if (IsModelFile(ext))                      RenderModelDetail(path);
    else if (IsAnimationFile(ext))                  RenderAnimationDetail(path);
    else                                            ImGui::TextDisabled((const char*)u8"プレビューなし");
}

void Editor::RenderDetail()
{
    if (selectedGameObject)
    {
        if (ImGui::Button((const char*)u8"追加", ImVec2(-FLT_MIN, 30))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::BeginMenu((const char*)u8"物理"))
            {
                if (ImGui::MenuItem((const char*)u8"RigidBody"))
                {

                }

                if (ImGui::MenuItem((const char*)u8"ボックスコライダー"))
                {

                }

                if (ImGui::MenuItem((const char*)u8"スフィアコライダー"))
                {

                }

                if (ImGui::MenuItem((const char*)u8"カプセルコライダー"))
                {

                }

                if (ImGui::MenuItem((const char*)u8"メッシュコライダー"))
                {

                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu((const char*)u8"ライト"))
            {
                if (ImGui::MenuItem((const char*)u8"ポイントライト"))
                {
                    selectedGameObject->AddComponent<PointLight>();
                }

                if (ImGui::MenuItem((const char*)u8"スポットライト"))
                {
                    selectedGameObject->AddComponent<SpotLight>();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu((const char*)u8"他"))
            {
                if (ImGui::MenuItem((const char*)u8"モデル")) {
					selectedGameObject->AddComponent<Model>();
                }

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();

        selectedGameObject->OnInspector();
    }
    else if (!selectedAsset.empty())
    {
        RenderAssetDetail(selectedAsset);
    }
}

void Editor::Render(Scene* scene, float fps)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowSize(ImVec2(500, 800), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.4f);

    // Overlay
    //ImGui::SetNextWindowPos(vp->WorkPos);
    //ImGui::SetNextWindowViewport(vp->ID);
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
    if (ImGui::Begin("エディット",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoDocking))
    {
        float upperHeight = ImGui::GetContentRegionAvail().y * 0.45f;

        if (ImGui::BeginMainMenuBar())
        {
            ImGui::SetNextItemWidth(200);

            if (ImGui::BeginCombo(("##" + currentScenePath.string()).c_str(),
                currentScenePath.empty() ? "<none>" : currentScenePath.string().c_str()))
            {
                if (ImGui::Selectable("<none>", currentScenePath.empty()))
                {
                    currentScenePath.clear();

					// 空のシーンをロード
                    scene->Load("empty.scene");
                    ImGui::EndCombo();
                    ImGui::EndMainMenuBar();
                    ImGui::End();
                    return;
                }

                auto sceneList = AssetManager::Instance().ScanAssets(".scene");
                for (auto& s : sceneList)
                {
                    std::string rel = s.filename().stem().string();
                    bool selected = (rel == currentScenePath);

                    if (ImGui::Selectable(rel.c_str(), selected))
                    {
                        currentScenePath = rel;

                        auto path = std::filesystem::path("Data/Scene/") / currentScenePath.filename();
                        scene->Load(path.string() + ".scene");
                        ImGui::EndCombo();
                        ImGui::EndMainMenuBar();
                        ImGui::End();
                        return;
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button((const char*)u8"保存"))
            {
                if (currentScenePath.empty()) {

					// 新規シーンの場合は「名前を付けて保存」を呼び出す
                    SaveSceneAs(scene);
                }
                else {

					// 既存シーンの場合は上書き保存
                    SaveScene(scene);
                }
            }

            if (ImGui::Button((const char*)u8"別名で保存"))
            {
				// 「名前を付けて保存」を呼び出す
                SaveSceneAs(scene);
            }

            ImGui::Separator();

            if (ImGui::Button((const char*)u8"レンダリング"))
            {
                ImGui::OpenPopup("RenderingSettings");
            }

            if (ImGui::BeginPopup("RenderingSettings"))
            {
				ImGui::Text((const char*)u8"スカイ");
                scene->GetSkyMapRenderer()->OnInspector();

                ImGui::Separator();

                ImGui::Text((const char*)u8"シャドウ");
                scene->GetShadowMapRenderer()->OnInspector();

				ImGui::Separator();

				ImGui::Text((const char*)u8"ポストエフェクト");
				scene->GetPostEffect()->OnInspector();

                ImGui::EndPopup();
            }

            ImGui::EndMainMenuBar(); // end of menu bar
        }

        // 上：タブ付きパネル
        ImGui::BeginChild("_upperPanel", ImVec2(0, upperHeight), true);

        if (ImGui::BeginTabBar("LeftTabBar"))
        {
            if (ImGui::BeginTabItem((const char*)u8"シーンデータ"))
            {
                ImGui::BeginChild("HierarchyRegion",
                    ImVec2(0, 0),
                    true,   // 枠を有効
                    ImGuiWindowFlags_NoMove);

                if (ImGui::Button((const char*)u8"追加", ImVec2(-FLT_MIN, 30))) {
                    scene->AddGameObject("New Game Object");
                }

                RenderGameObjectTree(scene, scene->GetRootObjects());

                if (ImGui::BeginDragDropTargetCustom(
                    ImGui::GetCurrentWindow()->Rect(),
                    ImGui::GetID("HierarchyRegion")))
                {
                    if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload("DND_GAMEOBJECT"))
                    {
                        GameObject* dragged =
                            *(GameObject**)payload->Data;

                        dragged->SetParent(scene, nullptr, true);
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem((const char*)u8"データ"))
            {
                RenderDataBrowser();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();

        // 下：詳細
        ImGui::BeginChild("_detailPanel", ImVec2(0, 0), true);
        ImGui::Text((const char*)u8"詳細情報");
        ImGui::Separator();
        RenderDetail();
        ImGui::EndChild();

        ImGui::End();
    }

    // Overlay
    //ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(400, 200));
    //ImGui::SetNextWindowViewport(vp->ID);
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
    if (ImGui::Begin("_overlay",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoDocking))
    {
        float width = ImGui::GetWindowWidth();

        if (IsEditing())
        {
            if (ImGui::Button(ICON_FA_PLAY, { 30, 30 }))
                EnterPlay(scene);

            ImGui::SameLine();

            ImGui::BeginDisabled();
            ImGui::Button(ICON_FA_STOP, { 30, 30 });
            ImGui::EndDisabled();
        }
        else if (IsPlaying())
        {
            if (ImGui::Button(ICON_FA_PAUSE, { 30, 30 }))
                EnterPause(scene);

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_STOP, { 30, 30 }))
                EnterEdit(scene);
        }
        else if (IsPaused())
        {
            if (ImGui::Button(ICON_FA_PLAY, { 30, 30 }))
                EnterResume(scene);

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_STOP, { 30, 30 }))
                EnterEdit(scene);
        }

        ImGui::Checkbox("Gizmos", &showGizmos);

        ImGui::Text("FPS %.0f", fps);

        // Editモードのとき
        if (!IsPlaying())
        {
            static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);

            auto DrawGizmoTool = [&](const char* icon, ImGuizmo::OPERATION op)
                {
                    const bool selected = (mCurrentGizmoOperation == op);

                    if (selected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
                    }

                    if (ImGui::Button(icon, ImVec2(40, 40)))
                        mCurrentGizmoOperation = op;

                    if (selected)
                        ImGui::PopStyleColor(3);

                    // ツールチップ
                    if (ImGui::IsItemHovered())
                    {
                        const char* name =
                            (op == ImGuizmo::TRANSLATE) ? "Move (W)" :
                            (op == ImGuizmo::ROTATE) ? "Rotate (E)" :
                            "Scale (R)";
                        ImGui::SetTooltip("%s", name);
                    }
                };

            DrawGizmoTool(ICON_FA_ARROWS_ALT, ImGuizmo::TRANSLATE);
            ImGui::SameLine();
            DrawGizmoTool(ICON_FA_SYNC_ALT, ImGuizmo::ROTATE);
            ImGui::SameLine();
            DrawGizmoTool(ICON_FA_EXPAND_ARROWS_ALT, ImGuizmo::SCALE);

            if (mCurrentGizmoOperation != ImGuizmo::SCALE)
            {
                if (ImGui::RadioButton((const char*)u8"ローカル", mCurrentGizmoMode == ImGuizmo::LOCAL))
                    mCurrentGizmoMode = ImGuizmo::LOCAL;
                ImGui::SameLine();
                if (ImGui::RadioButton((const char*)u8"ワールド", mCurrentGizmoMode == ImGuizmo::WORLD))
                    mCurrentGizmoMode = ImGuizmo::WORLD;
            }
            ImGui::SameLine();

            ImGuizmo::SetRect(vp->Pos.x, vp->Pos.y, vp->Size.x, vp->Size.y);

            if (selectedGameObject)
            {
				auto mc = scene->GetMainCamera();
                auto v = mc->GetView();
                auto p = mc->GetProjection();
                auto matrix = selectedGameObject->transform.GetMatrix();
                if (ImGuizmo::Manipulate(
                    (float*)&v,
                    (float*)&p,
                    mCurrentGizmoOperation,
                    mCurrentGizmoMode,
                    (float*)&matrix
                )) {
                    selectedGameObject->transform = Transform(matrix);
                }
            }
        }

        ImGui::End();
    }

    // エディターモードのみ実行、そしてマウスがUIにキャプチャされていないときのみカメラ操作を受け付ける
    if (!IsPlaying())
    {
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            UpdateCamera(scene);
        }
    }

#if 0 // --- ImGuizmo描画用グリッド ---
    DirectX::XMFLOAT4X4 view = camera.GetView();
    DirectX::XMFLOAT4X4 proj = camera.GetProjection();
    auto identity = DirectX::XMMatrixIdentity();
    ImGuizmo::DrawGrid((float*)&view, (float*)&proj, (float*)&identity, 100.f);
#endif

    if (showGizmos)
    {
        // --- シーン内の全オブジェクトに対してGizmos描画 ---
        std::function<void(GameObject*)> _render = [&](GameObject* obj)
            {
                obj->RenderGizmos(scene, obj == selectedGameObject);

                for (auto& child : obj->children)
                    _render(child.get());
            };

        for (auto& root : scene->GetRootObjects())
            _render(root.get());
    }
}
