#include "Skinning.hlsli"
#include "Phong.hlsli"

VS_OUT main(
    float4 position : POSITION,
    float4 boneWeights : BONE_WEIGHTS,
    uint4 boneIndices : BONE_INDICES,
    float2 texcoord : TEXCOORD,
    float3 normal : NORMAL,
    float3 tangent : TANGENT)
{
    VS_OUT vout = (VS_OUT) 0;
    position = SkinningPosition(position, boneWeights, boneIndices);
    vout.vertex = mul(position, viewProjection);
    vout.texcoord = texcoord;
    vout.normal = SkinningVector(normal, boneWeights, boneIndices);
    vout.position = position.xyz; // 視線ベクトルを求めるためにスキニング後のワールド座標をピクセルシェーダーに渡す
    vout.tangent = SkinningVector(tangent, boneWeights, boneIndices);
    
    // シャドウマップ
    float4 shadow = mul(position, lightViewProjection); // ライトから見た NDC 座標を算出
    shadow.xyz /= shadow.w;
    shadow.y = -shadow.y;
    shadow.xy = shadow.xy * 0.5f + 0.5f; // NDC 座標を UV 座標に変換
    vout.shadow = shadow.xyz;
    
    return vout;
}