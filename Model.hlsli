#include "Skinning.hlsli"
#include "Lights.hlsli"

struct VS_IN
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 bone_weights : WEIGHTS;
    uint4 bone_indices : BONES;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 world_position : POSITION;
    float4 world_normal : NORMAL;
    float4 world_tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 shadow : SHADOW;
};

cbuffer OBJECT_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 world;
};

cbuffer SCENE_CONSTANT_BUFFER : register(b2)
{
    row_major float4x4 view_projection;
    row_major float4x4 light_view_projection;
    float3 camera_position;
    float shadowTexelSize;
};

cbuffer LIGHT_CONSTANT_BUFFER : register(b3)
{
    DirectionLight directionLight;
    PointLight pointLight[10];
    SpotLight spotLight[10];
};
