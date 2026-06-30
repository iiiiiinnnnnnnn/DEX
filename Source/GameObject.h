#pragma once
#include <Common.h>
#include <Component.h>
#include "Transform.h"
class Scene;

class GameObject {
private:
    bool dirty = true; // プロパティに変更があった

    friend class Editor;
    void MarkDirty();
    void OnInspector();

    bool isActive = true;

public:
    std::string name;
    Scene* scene;
    GameObject* parent;
    Transform transform;

    std::vector<std::unique_ptr<GameObject>> children;
    std::vector<std::unique_ptr<Component>> components;

    GameObject(
        std::string name = "noname",
        Scene* scene = nullptr,
        GameObject* parent = nullptr,
        Vector3 position = Vector3::Zero,
        Quaternion rotation = Quaternion::Identity,
        Vector3 scale = Vector3::One) :
        name(name),
        scene(scene),
        parent(parent),
        transform(Transform(position, rotation, scale))
    {}

    bool IsActive() {
        return isActive;
    }
    void SetActive(bool active) {
        isActive = active;
	}

    Transform GetWorldTransform()
    {
        Matrix world = transform.GetMatrix();
        GameObject* p = parent;
        while (p)
        {
            world = world * p->transform.GetMatrix();
            p = p->parent;
        }
        return Transform(world);
    }

    void SetWorldTransform(const Transform& worldTransform)
    {
        Matrix world = worldTransform.GetMatrix();

        if (parent)
        {
            // 親のワールド行列を取得
            Matrix parentWorld = parent->GetWorldTransform().GetMatrix();

            // ローカル = 親ワールドの逆 * ワールド
            Matrix parentWorldInv = DirectX::XMMatrixInverse(nullptr, parentWorld);
            Matrix local = world * parentWorldInv;

            transform = Transform(local);
        }
        else
        {
            // 親がいなければローカル = ワールド
            transform = Transform(world);
        }

        dirty = true;
    }

    GameObject* AddGameObject(std::string name = "noname",
        Vector3 position = Vector3::Zero,
        Quaternion rotation = Quaternion::Identity,
        Vector3 scale = Vector3::One)
    {
        auto com = std::make_unique<GameObject>(name, scene, parent, position, rotation, scale);
        auto ptr = com.get();
        children.push_back(std::move(com));
        return ptr;
    }

    void SetParent(Scene* scene, GameObject* newParent, bool keepWorld);

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        comp->owner = this;
        T* ptr = comp.get();
        components.push_back(std::move(comp));
        return ptr;
    }

    template<typename T>
    T* GetComponent() {
        for (auto& c : components) {
            if (auto* t = dynamic_cast<T*>(c.get())) return t;
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
		std::vector<T*> result;
        for (auto& c : components) {
            if (auto* t = dynamic_cast<T*>(c.get())) result.push_back(t);
        }
        return result;
    }

    template<typename T>
    bool TryGetComponent(T** output) {
        for (auto& c : components) {
            if (auto* t = dynamic_cast<T*>(c.get()))
            {
                *output = t;
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool TryGetComponents(std::vector<T*>* output) {
        for (auto& c : components) {
            if (auto* t = dynamic_cast<T*>(c.get())) output->push_back(t);
        }
        if (output->empty())
            return false;
        return true;
    }

    // 毎フレーム
    virtual void Update(float elapsedTime) {
        for (auto& c : components)
            c->Update(elapsedTime);
    }

    // 描画
    virtual void RenderGizmos(Scene* scene, bool isSelected) {
        for (auto& c : components)
            c->RenderGizmos(scene, isSelected);
    }

    // 物理後とか
    virtual void LateUpdate(float elapsedTime) {
        for (auto& c : components)
            c->LateUpdate(elapsedTime);
    }

    // GameObject に追加
    void Save(json& j) const;

    GameObject* Load(const json& j, Scene* scene, GameObject* parent);
};