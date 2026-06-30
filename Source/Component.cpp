#include "Component.h"
#include <imgui.h>
#include <ImGuiRenderer.h>
#include <Graphics.h>
#include "GameObject.h"

void Component::OnErrorInspector()
{
	ImGui::PushID(this);
	if (ImGui::CollapsingHeader((const char*)u8"エラーコンポーネント", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreePush();
		//ImGui::TextColored({ 1,0,0,1 }, errorMessage.c_str());
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void Component::_RenderBillboard(Scene* scene, Color color, std::string filename)
{
    if (!billboard)
    {
		billboard = std::make_unique<Billboard>(Graphics::Instance().GetDevice(), filename);
    }

    billboard->Render(scene, owner->GetWorldTransform().position, 0.4f, color);
}