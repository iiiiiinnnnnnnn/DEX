#include "Phong.hlsli"

VS_OUT main(float4 position : POSITION)
{
    VS_OUT vout = (VS_OUT) 0;
    vout.vertex = mul(position, viewProjection);
    return vout;
}