#include "Light.h"
#include "GameObject.h"
#include "Scene.h"

void DirectionalLight::OnInspector()
{
	ImGui::ColorEdit3((const char*)u8"色", &color.x);
	ImGui::DragFloat((const char*)u8"強度", &intensity, 0.1f, 0.0f, 10.0f);
}

void PointLight::OnInspector()
{
	ImGui::ColorEdit3((const char*)u8"色", &color.x);
	ImGui::DragFloat((const char*)u8"範囲", &range, 0.1f);
	ImGui::DragFloat((const char*)u8"強度", &intensity, 0.1f, 0.0f, 10.0f);
}

void SpotLight::OnInspector()
{
	ImGui::ColorEdit3((const char*)u8"色", &color.x);
	ImGui::DragFloat((const char*)u8"範囲", &range, 0.1f);
	ImGui::DragFloat((const char*)u8"InnerCorn", &innerCorn, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat((const char*)u8"OuterCorn", &outerCorn, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat((const char*)u8"強度", &intensity, 0.1f, 0.0f, 10.0f);
}

void DirectionalLight::RenderGizmos(Scene* scene, bool isSelected)
{
	std::string filename = "sun.dds";

	_RenderBillboard(
		scene,
		_RGB(color * intensity),
		filename);
}

void PointLight::RenderGizmos(Scene* scene, bool isSelected)
{
	std::string filename = "pointlight.dds";

	_RenderBillboard(
		scene,
		_RGB(color * intensity),
		filename);
}

void SpotLight::RenderGizmos(Scene* scene, bool isSelected)
{
	std::string filename = "spotlight.dds";

	_RenderBillboard(
		scene,
		_RGB(color * intensity),
		filename);
}
