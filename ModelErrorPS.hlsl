// Error model shader

#include "Model.hlsli"

float4 main(VS_OUT pin) : SV_TARGET
{
    return float4(1, 0.2f, 1, 1);
}