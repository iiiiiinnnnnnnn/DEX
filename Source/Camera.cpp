#include "Camera.h"
#include "GameObject.h"
#include "Scene.h"

//----------------------------------------
// View
//----------------------------------------
Matrix Camera::GetView() const
{
	const Transform& t = owner->transform;
	return t.GetMatrix().Invert();
}

//----------------------------------------
// Direction vectors
//----------------------------------------
Vector3 Camera::GetFront() const
{
	const auto& t = owner->transform;
	return Vector3::Transform(Vector3::Forward, t.rotation);
}

Vector3 Camera::GetRight() const
{
	const auto& t = owner->transform;
	return Vector3::Transform(Vector3::Right, t.rotation);
}

Vector3 Camera::GetUp() const
{
	const auto& t = owner->transform;
	return Vector3::Transform(Vector3::Up, t.rotation);
}

bool Camera::WorldToScreen(
	const Vector3& world,
	Vector2& out,
	float screenWidth,
	float screenHeight) const
{
	Matrix view = GetView();
	Matrix proj = GetProjection();
	Matrix vp = view * proj;

	Vector4 clip(world.x, world.y, world.z, 1.0f);
	clip = Vector4::Transform(clip, vp);

	if (clip.w <= 0.0f)
		return false;

	Vector3 ndc = Vector3(clip.x, clip.y, clip.z) / clip.w;

	out.x = (ndc.x * 0.5f + 0.5f) * screenWidth;
	out.y = (-ndc.y * 0.5f + 0.5f) * screenHeight;

	return true;
}

void Camera::CopyTo(Camera* dst)
{
	// ƒJƒƒ‰ƒpƒ‰ƒ[ƒ^
	dst->SetPerspectiveFov(
		fovY,
		aspect,
		nearZ,
		farZ
	);

	dst->owner->transform = owner->transform;
}

//----------------------------------------
// Inspector
//----------------------------------------
void Camera::OnInspector()
{
	ImGui::TreePush();

	float fovDeg = DirectX::XMConvertToDegrees(fovY);
	if (ImGui::DragFloat("FOV", &fovDeg, 0.1f, 1.0f, 179.0f))
	{
		fovDeg = std::clamp(fovDeg, 1.0f, 179.0f);
		fovY = DirectX::XMConvertToRadians(fovDeg);
	}

	if (ImGui::DragFloat("Near", &nearZ, 0.01f))
	{
		nearZ = std::max(nearZ, 0.001f); // 0–hŽ~
		nearZ = std::min(nearZ, farZ - 0.01f); // far‚Æ‚ÌŠÖŒW
	}

	if (ImGui::DragFloat("Far", &farZ, 1.0f))
	{
		farZ = std::max(farZ, nearZ + 0.01f); // near‚Æ‚ÌŠÖŒW
	}

	ImGui::TreePop();
}

void Camera::RenderGizmos(Scene* scene, bool isSelected)
{
	_RenderBillboard(
		scene,
		scene->GetMainCamera() == this ? Color(255, 0, 0) : Color(1, 1, 1), // ƒƒCƒ“ƒJƒƒ‰‚È‚çÔ
		"camera.dds");
}

//----------------------------------------
// LookAt
//----------------------------------------
void Camera::SetLookAt(const Vector3& eye, const Vector3& focus, const Vector3& up)
{
	auto& t = owner->transform;

	Vector3 forward = focus - eye;
	forward.Normalize();

	Matrix world = Matrix::CreateWorld(
		eye,
		forward,
		up
	);

	Vector3 scale;
	Quaternion rot;
	Vector3 pos;

	world.Decompose(scale, rot, pos);

	t.position = pos;
	t.rotation = rot;
}

//----------------------------------------
// PerspectiveÝ’è
//----------------------------------------
void Camera::SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ)
{
	this->fovY = fovY;
	this->aspect = aspect;
	this->nearZ = nearZ;
	this->farZ = farZ;
}