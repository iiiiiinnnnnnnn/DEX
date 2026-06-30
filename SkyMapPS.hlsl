#include "SkyMap.hlsli"

Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);

//--------------------------------------------
//	パノラマスカイボックス
//--------------------------------------------
// tex:パノラマスカイボックス用テクスチャ
// samp: パノラマスカイボックス用サンプラステート
//direction:方向ベクトル(正規化済み)
float4 SampleSkybox(Texture2D tex, SamplerState samp, float3 direction)
{
    static const float PI = 3.14159265f;
    float latitude = atan2(direction.z, direction.x) * (1.0f / (2.0f * PI)) + 0.5f;
    float longitude = atan2(direction.y, length(direction.xz)) * (1.0f / PI) + 0.5f;
    return tex.SampleLevel(samp, float2(1.0f - latitude, 1.0f - longitude), 0);
}

float4 main(VS_OUT pin) : SV_TARGET
{
    float3 E = normalize(pin.world_position.xyz - cameraPosition.xyz);

    //  スカイボックスから色を取得する
    return SampleSkybox(spriteTexture, spriteSampler, E);
}
