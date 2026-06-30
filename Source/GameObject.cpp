#include "GameObject.h"
#include "Scene.h"
#include "Model.h"

void GameObject::SetParent(Scene* scene, GameObject* newParent, bool keepWorld)
{
    if (parent == newParent)
        return;

    Transform world;
    if (keepWorld)
        world = GetWorldTransform();

    std::unique_ptr<GameObject> selfPtr;

    if (parent)
    {
        auto& siblings = parent->children;

        for (auto it = siblings.begin(); it != siblings.end(); ++it)
        {
            if (it->get() == this)
            {
                selfPtr = std::move(*it);
                siblings.erase(it);
                break;
            }
        }
    }
    else
    {
        auto& roots = scene->GetRootObjects();

        for (auto it = roots.begin(); it != roots.end(); ++it)
        {
            if (it->get() == this)
            {
                selfPtr = std::move(*it);
                roots.erase(it);
                break;
            }
        }
    }

    parent = newParent;

    if (newParent)
    {
        newParent->children.push_back(std::move(selfPtr));
    }
    else
    {
        scene->rootObjects.push_back(std::move(selfPtr));
    }

    if (keepWorld)
        SetWorldTransform(world);

    MarkDirty();
}

void GameObject::Save(json& j) const
{
    j["name"] = name;

    transform.Save(j);

    auto& comps = j["components"];
    for (const auto& c : components) {
        json comp_json;
        c->Serialize(comp_json);
        comps.push_back(std::move(comp_json));
    }

    auto& children_json = j["children"];
    for (auto& child : children) {
        json child_json;
        child->Save(child_json);
        children_json.push_back(std::move(child_json));
    }
}

GameObject* GameObject::Load(const json& j, Scene* scene, GameObject* parent)
{
    return nullptr;
}

void GameObject::MarkDirty()
{
    dirty = true;
    for (auto& c : children)
        c->MarkDirty();
}

void GameObject::OnInspector()
{
    if (ImGui::CollapsingHeader(
        (std::string(ICON_FA_CUBE) + (const char*)u8" ゲームオブジェクト").c_str(),
        ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TreePush();

        ImGui::InputText((const char*)u8"名前", &name);
        ImGui::Text((const char*)u8"子オブジェクト数: %d", (int)children.size());

        ImGui::TreePop();
    }

    transform.OnInspector(dirty);

    for (size_t i = 0; i < components.size();)
    {
        Component* c = components[i].get();

        ImGui::PushID(c);

        if (ImGui::CollapsingHeader(c->GetLabel().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 並び替え
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("COMPONENT", &i, sizeof(size_t));
                ImGui::Text((const char*)u8"(ドロップで並び替え)");
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
                {
                    size_t src = *(size_t*)payload->Data;
                    std::swap(components[src], components[i]);
                }
                ImGui::EndDragDropTarget();
            }

            // 右クリックメニュー
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem(
                    (std::string(ICON_FA_TRASH) + (const char*)u8" 削除").c_str()))
                {
                    components.erase(components.begin() + i);
                    ImGui::EndPopup();
                    ImGui::PopID();
                    continue;
                }

                ImGui::EndPopup();
            }

            ImGui::TreePush(c);

            if (c->IsValid()) {
                c->OnInspector();
            }
            else {
                c->OnErrorInspector();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
        i++;
    }
}