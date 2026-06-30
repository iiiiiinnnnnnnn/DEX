#pragma once

#include <Common.h>

struct Transform
{
    Vector3 position = Vector3::Zero;
    Quaternion rotation = Quaternion::Identity;
    Vector3 scale = Vector3::One;

    Transform() = default;

    Transform(Matrix matrix) {
        matrix.Decompose(scale, rotation, position);
    }

    Transform(Vector3 position, Quaternion rotation, Vector3 scale)
        : position(position), rotation(rotation), scale(scale) { }

    inline Matrix GetMatrix() const
    {
        return
            Matrix::CreateScale(scale) *
            Matrix::CreateFromQuaternion(rotation) *
            Matrix::CreateTranslation(position);
    }

    void OnInspector(bool& dirty)
    {
        if (ImGui::CollapsingHeader((std::string(ICON_FA_ARROWS_ALT) + (const char*)u8" トランスフォーム (ローカル)").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TreePush();

            // position
            if (ImGui::DragFloat3((const char*)u8"位置", &position.x, 0.1f)) {
                dirty = true;
            }

            // rotation
            {
                Vector3 euler = rotation.ToEuler();
                Vector3 eulerDeg = euler * RAD2DEG;

                if (ImGui::DragFloat3((const char*)u8"回転(度)", &eulerDeg.x, 0.5f))
                {
                    Vector3 rad = eulerDeg * DEG2RAD;

                    rotation = Quaternion::CreateFromYawPitchRoll(
                        rad.y,
                        rad.x,
                        rad.z
                    );

                    dirty = true;
                }
            }

            // scale
            if (ImGui::DragFloat3((const char*)u8"スケール", &scale.x, 0.1f)) {
                dirty = true;
            }

            ImGui::TreePop();
        }
    }

    void Save(json& j) const {
        j["p"] = { position.x, position.y, position.z };
        j["r"] = { rotation.x, rotation.y, rotation.z, rotation.w };
        j["s"] = { scale.x, scale.y, scale.z };
    }

    void Load(const json& j) {
        position = Vector3(j["p"][0], j["p"][1], j["p"][2]);
        rotation = Quaternion(j["r"][0], j["r"][1], j["r"][2], j["r"][3]);
        scale = Vector3(j["s"][0], j["s"][1], j["s"][2]);
    }
};