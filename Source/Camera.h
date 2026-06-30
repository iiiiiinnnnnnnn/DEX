#pragma once

#include <Common.h>
#include "Component.h"

class Scene;

// カメラ
class Camera : public Component {
private:
	friend class GameObject;
	void OnInspector() override;
public:
	Camera() = default;

	DEFINE_COMPONENT_TYPE(ICON_FA_CAMERA, カメラ)

	void RenderGizmos(Scene* scene, bool isSelected) override;    // 描画

	// 指定方向を向く
	void SetLookAt(const Vector3& eye, const Vector3& focus, const Vector3& up);

	// パースペクティブ設定
	void SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

	// プロジェクション行列
	Matrix GetProjection() const
	{
		return Matrix::CreatePerspectiveFieldOfView(
			fovY,
			aspect,
			nearZ,
			farZ
		);
	}

	Matrix GetView() const;

	Vector3 GetFront() const;

	Vector3 GetRight() const;

	Vector3 GetUp() const;

	bool WorldToScreen(
		const Vector3& world,
		Vector2& out,
		float screenWidth,
		float screenHeight) const;

	void CopyTo(Camera* dst);

	void Serialize(json& j) override {
		Component::Serialize(j);
		j["fovY"] = fovY;
		j["aspect"] = aspect;
		j["nearZ"] = nearZ;
		j["farZ"] = farZ;
	}

	void Deserialize(const json& j) override {
		Component::Deserialize(j);
		if (j.contains("fovY")) fovY = j["fovY"].get<float>();
		if (j.contains("aspect")) aspect = j["aspect"].get<float>();
		if (j.contains("nearZ")) nearZ = j["nearZ"].get<float>();
		if (j.contains("farZ")) farZ = j["farZ"].get<float>();
	}

private:
	float fovY = DirectX::XM_PIDIV4;
	float aspect = 16.0f / 9.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
};