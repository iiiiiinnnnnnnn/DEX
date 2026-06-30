static const uint MAX_BONES = 256;

cbuffer SKINNING_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 bone_transforms[MAX_BONES];
};

float4 SkinningPosition(float4 position, float4 boneWeights, uint4 boneIndices)
{
    float4 p = float4(0, 0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        p += (boneWeights[i] * mul(position, bone_transforms[boneIndices[i]]));
    }
    return p;
}

float3 SkinningVector(float3 vec, float4 boneWeights, uint4 boneIndices)
{
    float3 v = float3(0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        v += boneWeights[i] * mul(float4(vec, 0), bone_transforms[boneIndices[i]]).xyz;
    }
    return v;
}
