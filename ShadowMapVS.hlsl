#include "Skinning.hlsli"

cbuffer OBJECT_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 world;
};

cbuffer SCENE_CONSTANT_BUFFER : register(b2)
{
    row_major float4x4 lightViewProjection;
};

struct VS_IN
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 bone_weights : WEIGHTS;
    uint4 bone_indices : BONES;
};

float4 main(VS_IN vin) : SV_POSITION
{
    float4 pos = SkinningPosition(vin.position, vin.bone_weights, vin.bone_indices);
    pos = mul(pos, world);
    return mul(pos, lightViewProjection);
}