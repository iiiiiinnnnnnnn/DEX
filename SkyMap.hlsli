struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float4 world_position : WORLD_POSITION;
};

cbuffer CbSkyMap : register(b0)
{
    row_major float4x4 inverseViewProjection;
    float3 cameraPosition;
    float _dummy;
};