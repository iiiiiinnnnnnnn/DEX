#pragma once

#include <memory>
#include "Sprite.h"
#include "Camera.h"

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