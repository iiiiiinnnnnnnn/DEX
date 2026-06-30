#include "Billboard.hlsli"

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

float4 main(IN input) : SV_TARGET
{
    float4 tex = tex0.Sample(samp0, input.uv);
    return tex * input.color;
}