#pragma once

#include "Component.h"

class GameObject;
class Scene;

class DirectionalLight : public Component {
private:
	friend class GameObject;
	friend class Scene;
	void OnInspector() override;
	void RenderGizmos(Scene* scene, bool isSelected) override;

	void Serialize(json& j) override {
		Component::Serialize(j);
		j["color"] = { color.R(), color.G(), color.B() };
		j["intensity"] = intensity;

	}

	void Deserialize(const json& j) override {
		Component::Deserialize(j);
		auto arr = j["color"];
		color = Color(arr[0], arr[1], arr[2]);
		intensity = j["intensity"];
	}

public:
	DEFINE_COMPONENT_TYPE(ICON_FA_SUN, ディレクショナルライト)

	Color color = { 1,1,1,1 };
	float intensity = 1.0f;
	DirectionalLight() {}
	DirectionalLight(Color color, float intensity)
	{
		this->color = color;
		this->intensity = intensity;
	}
};

class PointLight : public Component {
private:
	friend class GameObject;
	friend class Scene;
	void OnInspector() override;
	void RenderGizmos(Scene* scene, bool isSelected) override;

	void Serialize(json& j) override {
		Component::Serialize(j);
		j["color"] = { color.R(), color.G(), color.B() };
		j["range"] = range;
		j["intensity"] = intensity;
	}

	void Deserialize(const json& j) override {
		Component::Deserialize(j);
		auto arr = j["color"];
		color = Color(arr[0], arr[1], arr[2]);
		range = j["range"];
		intensity = j["intensity"];
	}

public:
	DEFINE_COMPONENT_TYPE(ICON_FA_LIGHTBULB, ポイントライト)

	Color color = { 1,1,1,1 };
	float range = 10;
	float intensity = 1.0f;
	PointLight() {}
	PointLight(Color color, float range, float intensity)
	{
		this->color = color;
		this->range = range;
		this->intensity = intensity;
	}
};

class SpotLight : public Component {
private:
	friend class GameObject;
	friend class Scene;
	void OnInspector() override;
	void RenderGizmos(Scene* scene, bool isSelected) override;

	void Serialize(json& j) override {
		Component::Serialize(j);
		j["color"] = { color.R(), color.G(), color.B() };
		j["range"] = range;
		j["innerCorn"] = innerCorn;
		j["outerCorn"] = outerCorn;
		j["intensity"] = intensity;
	}

	void Deserialize(const json& j) override {
		Component::Deserialize(j);
		auto arr = j["color"];
		color = Color(arr[0], arr[1], arr[2]);
		range = j["range"];
		innerCorn = j["innerCorn"];
		outerCorn = j["outerCorn"];
		intensity = j["intensity"];
	}

public:
	DEFINE_COMPONENT_TYPE(ICON_FA_LIGHTBULB, スポットライト)

	Color color = { 1,1,1,1 };
	float range = 10;
	float innerCorn = 0.99f;
	float outerCorn = 0.9f;
	float intensity = 1.0f;
	SpotLight() {}
	SpotLight(Color color, float range, float innerCorn, float outerCorn, float intensity)
	{
		this->color = color;
		this->range = range;
		this->innerCorn = innerCorn;
		this->outerCorn = outerCorn;
		this->intensity = intensity;
	}
};

struct DIRECTIONAL_LIGHT_STRUCT
{
	Vector3 direction;
	float intensity;
	Color color;
};

struct POINT_LIGHT_STRUCT
{
	Vector3 position;
	float range;
	Color color;
	float intensity;
	float DUMMY;
	float DUMMY;
	float DUMMY;
};

struct SPOT_LIGHT_STRUCT
{
	Vector3 position;
	float range;
	Vector3 direction;
	float innerCorn;
	Color color;
	float outerCorn;
	float intensity;
	float DUMMY;
	float DUMMY;
};
