#pragma once

#include <Common.h>
#include "GizmosRenderer.h"
#include "Billboard.h"

#define DEFINE_COMPONENT_TYPE(icon, name) \
	inline static std::string label = std::string(icon) + std::string(" ") + (const char*)u8#name; \
	inline static size_t typeHash = std::hash<std::string>{}(#name); \
	static const std::string& GetLabelStatic() { return label; } \
	static size_t GetTypeHashStatic() { return typeHash; } \
	const std::string& GetLabel() const override { return label; } \
	size_t GetTypeHash() const override { return typeHash; }

#define DEFINE_TYPE(icon, name) \
	inline static std::string label = std::string(icon) + std::string(" ") + (const char*)u8#name; \
	inline static size_t typeHash = std::hash<std::string>{}(#name); \
	static const std::string& GetLabelStatic() { return label; } \
	static size_t GetTypeHashStatic() { return typeHash; } \
	const std::string& GetLabel() const { return label; } \
	size_t GetTypeHash() const { return typeHash; }

class GameObject;
class Scene;

class Component {
public:
    Component() = default;

    GameObject* owner = nullptr;

	bool IsEnabled() const { return isEnabled; }
	void SetEnabled(bool enabled) { isEnabled = enabled; }
	bool IsValid() const { return isValid; }

    virtual const std::string& GetLabel() const = 0;
    virtual uint64_t GetTypeHash() const = 0;

private:
    friend class GameObject;
    friend class Scene;

    bool isEnabled = true;

    virtual void OnInspector() {}                    // ImGuiでプロパティ描画
    virtual void OnErrorInspector(); // ImGuiでプロパティ描画(エラー)

    virtual void Update(float elapsedTime) {}          // 毎フレーム
    virtual void LateUpdate(float elapsedTime) {}      // 物理後とか
    virtual void Render(Scene* scene) {}    // 描画
    virtual void RenderGizmos(Scene* scene, bool isSelected) {};    // 描画

protected:
    virtual void Serialize(json& j) {
        j["hash"] = GetTypeHash();
    }
    virtual void Deserialize(const json& j) {}

    virtual void _RenderBillboard(Scene* scene, Color color, std::string filename);

    bool isValid = true;
    std::unique_ptr<Billboard> billboard;
};