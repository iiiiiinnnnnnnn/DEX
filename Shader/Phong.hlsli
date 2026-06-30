#include "../Lights.hlsli"

struct VS_OUT
{
    float4 vertex : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 position : POSITION;
    float3 tangent : TANGENT;
    float3 shadow : SHADOW;
};

cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
    float4 cameraPosition;
    row_major float4x4 lightViewProjection;
    float4 shadowColor;
    float shadowTexelSize;
};

cbuffer CbLights : register(b1)
{
    float4 ambientColor;
    DirectionLight directionLight;
    PointLight pointLight[10];
    SpotLight spotLight[10];
};

cbuffer CbMesh : register(b2)
{
    float4 materialColor;
};
