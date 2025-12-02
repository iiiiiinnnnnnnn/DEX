#include "Scene.h"
#include "Graphics.h"

// コンストラクタ
SpriteTestScene::SpriteTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	sprites[0] = std::make_unique<Sprite>(device);
}

// 描画処理
void SpriteTestScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	sprites[0]->Render(dc, 200, 200, 400, 200, 45, 1, 0, 0, 1);
}