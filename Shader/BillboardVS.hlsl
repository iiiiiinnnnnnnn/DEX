#include "Billboard.hlsli"

cbuffer CameraCB : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 proj;
};

VS_OUT main(IN input)
{
    VS_OUT o;
    float4 worldPos = float4(input.pos, 1.0f);
    float4 viewPos = mul(worldPos, view);
    o.pos = mul(viewPos, proj);

    // 深度が奥に行きすぎて消える対策（必要なら調整）
    o.pos.z *= 0.999f;

    o.color = input.color;
    o.uv = input.uv;
    return o;
}