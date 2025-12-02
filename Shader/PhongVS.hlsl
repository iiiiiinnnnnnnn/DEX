#include "Phong.hlsli"

VS_OUT main(float4 position : POSITION, float2 texcoord : TEXCOORD)
{
    VS_OUT vout = (VS_OUT) 0;
    vout.vertex = mul(position, mul(worldTransform, viewProjection));
    vout.texcoord = texcoord;
    return vout;
}