#include "Scene.h"
#include "Graphics.h"

// コンストラクタ
SpriteTestScene::SpriteTestScene()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	sprites[0] = std::make_unique<Sprite>(device, "Data/Sprite/player-sprites.png");
	sprites[1] = std::make_unique<Sprite>(device);
}

// 描画処理
void SpriteTestScene::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ID3D11SamplerState* samplers[] =
	{
		renderState->GetSamplerState(SamplerState::PointClamp)
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);

	float width = 140;
	float height = 240;
	sprites[0]->Render(dc, 100, 50, width, height, 10 * width, 3 * height, width, height, 0, 1, 1, 1, 1);
	sprites[0]->Render(dc, 100, 350, 480, 300, 0, 1, 1, 1, 1);
	sprites[1]->Render(dc, 300, 50, width, height, 0, 0, 1, 0, 1);
}