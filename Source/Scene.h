#pragma once

#include <memory>
#include "Sprite.h"
#include "Camera.h"
#include "FreeCameraController.h"
#include "Model.h"
#include "Light.h"
#include "PostEffect.h"

// シーン基底
class Scene
{
public:
	Scene() = default;
	virtual ~Scene() = default;

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// 描画処理
	virtual void Render(float elapsedTime) {}
};

// スプライトテストシーン
class SpriteTestScene : public Scene
{
public:
	SpriteTestScene();
	~SpriteTestScene() override = default;

	// 描画処理
	void Render(float elapsedTime) override;

private:
	std::unique_ptr<Sprite> sprites[8];
};

// 深度テストシーン
class DepthTestScene : public Scene
{
public:
	DepthTestScene();
	~DepthTestScene() override = default;
	// 描画処理
	void Render(float elapsedTime) override;
private:
	std::unique_ptr<Sprite> sprite;
};

// ブレンドテストシーン
class BlendTestScene : public Scene
{
public:
	BlendTestScene();
	~BlendTestScene() override = default;
	// 描画処理
	void Render(float elapsedTime) override;
private:
	std::unique_ptr<Sprite> sprite;
};

// ラスタライズテストシーン
class RasterizeTestScene : public Scene
{
public:
	RasterizeTestScene();
	~RasterizeTestScene() override = default;
	// 描画処理
	void Render(float elapsedTime) override;
private:
	std::unique_ptr<Sprite> sprite;
};

// ギズモテストシーン
class GizmosTestScene : public Scene
{
public:
	GizmosTestScene();
	~GizmosTestScene() override = default;
	// 描画処理
	void Render(float elapsedTime) override;
private:
	Camera camera;
	float rotation = 0;
};


// モデルテストシーン
class ModelTestScene : public Scene
{
public:
	ModelTestScene();
	~ModelTestScene() override = default;

	// 描画処理
	void Render(float elapsedTime) override;

private:
	// シーンGUI描画
	void DrawSceneGUI();

	// プロパティGUI描画
	void DrawPropertyGUI();

	// アニメーションGUI描画
	void DrawAnimationGUI();

	Camera camera;
	std::unique_ptr<Model> model;
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 angle = { 0, 0, 0 };
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };

	Model::Node* selectionNode = nullptr;
	FreeCameraController cameraController;

	bool animationLoop = false;
	float animationBlendSeconds = 0.2f;

	LightManager lightManager;
};


// ポストエフェクトテストシーン
class PostEffectTestScene : public Scene
{
public:
	PostEffectTestScene();
	~PostEffectTestScene() override = default;
	// 描画処理
	void Render(float elapsedTime) override;
private:
	std::unique_ptr<Sprite> sprite;
	std::unique_ptr<PostEffect> postEffect;

	// ポストエフェクトGUI描画
	void DrawPostEffectGUI();
};


// シャドウテストシーン
class ShadowTestScene : public Scene
{
public:
	ShadowTestScene();
	~ShadowTestScene() override = default;
	// 描画処理
	void Render(float elapsedTime) override;
private:
	// シャドウマップGUI描画
	void DrawShadowMapGUI();
	Camera camera;
	FreeCameraController cameraController;
	LightManager lightManager;
	std::unique_ptr<Model> stage;
	std::unique_ptr<Model> character;
};