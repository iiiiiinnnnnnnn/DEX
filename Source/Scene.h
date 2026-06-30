#pragma once
#include <Common.h>
#include "GameObject.h"
#include "Sprite.h"
#include "Camera.h"
#include "FreeCameraController.h"
#include "Model.h"
#include "Light.h"
#include "SkyMapRenderer.h"
#include "PostEffect.h"
#include <RenderState.h>
#include <ShadowMapRenderer.h>
#include <GizmosRenderer.h>

// シーン基底
class Scene {
private:
	friend class GameObject;
	std::vector<std::unique_ptr<GameObject>> rootObjects;

	RenderState* renderState;
	Camera* mainCamera;

	std::unique_ptr<SkyMapRenderer> skyMapRenderer;
	std::unique_ptr<GizmosRenderer> gizmosRenderer;
	std::unique_ptr<ShadowMapRenderer> shadowMapRenderer;
	std::unique_ptr<PostEffect> postEffect;

	DirectionalLight* directionalLight;
	std::vector<PointLight*> pointLights;
	std::vector<SpotLight*> spotLights;

public:
	std::vector<std::unique_ptr<GameObject>>& GetRootObjects() { return this->rootObjects; }

	RenderState* GetRenderState() { return this->renderState; }
	Camera* GetMainCamera() { return this->mainCamera; }

	SkyMapRenderer* GetSkyMapRenderer() { return this->skyMapRenderer.get(); }
	GizmosRenderer* GetGizmosRenderer() { return this->gizmosRenderer.get(); }
	ShadowMapRenderer* GetShadowMapRenderer() { return this->shadowMapRenderer.get(); }
	PostEffect* GetPostEffect() { return this->postEffect.get(); }

	const DirectionalLight* GetDirectionalLight() { return this->directionalLight; }
	std::vector<PointLight*>& GetPointLights() { return this->pointLights; }
	std::vector<SpotLight*>& GetSpotLights() { return this->spotLights; }

	std::vector<Camera*> GetAllCameras()
	{
		std::vector<Camera*> cameras;

		std::function<void(GameObject*)> _getcam = [&](GameObject* obj)
			{
				Camera* cam;
				if (obj->TryGetComponent(&cam))
				{
					cameras.push_back(cam);
				}

				for (auto& child : obj->children)
					_getcam(child.get());
			};

		for (auto& root : this->rootObjects)
			_getcam(root.get());

		return cameras;
	}

	GameObject* AddGameObject(
		std::string name = "noname",
		Vector3 position = Vector3::Zero,
		Quaternion rotation = Quaternion::Identity,
		Vector3 scale = Vector3::One)
	{
		auto com = std::make_unique<GameObject>(name, this, nullptr, position, rotation, scale);
		auto ptr = com.get();
		this->rootObjects.push_back(std::move(com));
		return ptr;
	}

	void Initialize();
	void Finalize();

	void Update(float elapsedTime);
	void LateUpdate(float elapsedTime);
	void Render(float elapsedTime);

	void Save(std::filesystem::path path);
	void Load(std::filesystem::path path);

private:
	void DeleteAllObjects()
	{
		std::function<void(GameObject*)> _delete = [&](GameObject* obj)
			{
				obj->children.clear();

				for (auto& child : obj->children)
					_delete(child.get());
			};

		for (auto& root : this->rootObjects)
			_delete(root.get());
	}

	void UpdateAllObjects(float elapsedTime)
	{
		std::function<void(GameObject*)> _update = [&](GameObject* obj)
			{
				obj->Update(elapsedTime);

				for (auto& child : obj->children)
					_update(child.get());
			};

		for (auto& root : this->rootObjects)
			_update(root.get());
	}

	// ライトを全て集めてセット
	void CollectAndSetLights()
	{
		this->directionalLight = {};
		this->pointLights.clear();
		this->spotLights.clear();

		std::function<void(GameObject*)> _collect = [&](GameObject* obj)
			{
				DirectionalLight* directional;
				if (obj->TryGetComponent<DirectionalLight>(&directional)) {
					this->directionalLight = directional;
				}

				std::vector<PointLight*> points;
				if (obj->TryGetComponents<PointLight>(&points)) {
					for (auto l : points)
						this->pointLights.push_back(l);
				}

				std::vector<SpotLight*> spots;
				if (obj->TryGetComponents<SpotLight>(&spots)) {
					for (auto l : spots)
						this->spotLights.push_back(l);
				}

				for (auto& child : obj->children)
					_collect(child.get());
			};

		for (auto& root : this->rootObjects)
			_collect(root.get());
	}

	void RenderAllObjects()
	{
		std::function<void(GameObject*)> _render = [&](GameObject* obj)
			{
				for (auto& c : obj->components)
				{
					c->Render(this);
				}

				for (auto& child : obj->children)
					_render(child.get());
			};

		for (auto& root : this->rootObjects)
			_render(root.get());
	}

	void RenderAllShadows()
	{
		this->shadowMapRenderer->Begin(this, { 0,0,0 });

		std::function<void(GameObject*)> render = [&](GameObject* obj)
			{
				Model* model;
				if (obj->TryGetComponent<Model>(&model)) {
					this->shadowMapRenderer->Draw(this, model);
				}

				for (auto& child : obj->children)
					render(child.get());
			};

		for (auto& root : this->rootObjects)
			render(root.get());

		this->shadowMapRenderer->End();
	}

	// true : カメラが存在し、かつアクティブ
	bool CheckMainCamera()
	{
		if (!this->mainCamera || !this->mainCamera->owner->IsActive())
		{
			this->mainCamera = nullptr;
			auto cameras = GetAllCameras();
			for (auto cam : cameras)
			{
				if (cam->owner->IsActive())
				{
					this->mainCamera = cam;
					return true;
				}
			}

			return false;
		}
		return true;
	}

	void SaveGameObject(const GameObject* go, json& j)
	{
		j["name"] = go->name;

		// Transform 保存
		json tf;
		go->transform.Save(tf);
		j["transform"] = tf;

		// コンポーネント保存
		json& comps = j["components"];
		comps = json::array();

		for (const auto& comp : go->components)
		{
			json c;
			comp->Serialize(c);
			comps.push_back(c);
		}

		// 子オブジェクト
		json& children = j["children"];
		children = json::array();

		for (auto& child : go->children)
		{
			json child_json;
			SaveGameObject(child.get(), child_json);
			children.push_back(child_json);
		}
	}

	void LoadGameObject(const json& j, GameObject* parent)
	{
		auto go = std::make_unique<GameObject>();

		// 名前
		if (j.contains("name"))
			go->name = j["name"].get<std::string>();

		// --- Transform復元 ---
		if (j.contains("transform"))
		{
			go->transform.Load(j["transform"]);
		}

		// --- コンポーネント復元 ---
		if (j.contains("components"))
		{
			for (const auto& c : j["components"])
			{
				if (!c.contains("hash"))
					continue;

				size_t hash = c["hash"].get<size_t>();

				Component* comp = nullptr;

				if(hash == DirectionalLight::GetTypeHashStatic())
				{
					comp = go->AddComponent<DirectionalLight>();
				}
				else if (hash == PointLight::GetTypeHashStatic())
				{
					comp = go->AddComponent<PointLight>();
				}
				else if (hash == SpotLight::GetTypeHashStatic())
				{
					comp = go->AddComponent<SpotLight>();
				}
				else if (hash == Camera::GetTypeHashStatic())
				{
					comp = go->AddComponent<Camera>();
				}
				else if (hash == Model::GetTypeHashStatic())
				{
					comp = go->AddComponent<Model>();
				}

				if (!comp)
					return;

				comp->Deserialize(c);
			}
		}

		// --- 子オブジェクト ---
		if (j.contains("children"))
		{
			for (const auto& child_json : j["children"])
			{
				LoadGameObject(child_json, go.get());
			}
		}

		// 親子関係
		if (parent)
		{
			go->parent = parent;
			parent->children.push_back(std::move(go));
		}
		else
		{
			// ルートなら管理配列へ
			rootObjects.push_back(std::move(go));
		}

		return;
	}
};
