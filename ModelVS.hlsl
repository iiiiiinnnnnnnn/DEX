#include "Model.hlsli"

VS_OUT main(VS_IN vin)
{
    vin.normal.w = 0;
    float sigma = vin.tangent.w;
    float4 blended_position = { 0, 0, 0, 0 };
    float4 blended_normal = { 0, 0, 0, 0 };
    float4 blended_tangent = { 0, 0, 0, 0 };
    for (int bone_index = 0; bone_index < 4; ++bone_index)
    {
        blended_position += vin.bone_weights[bone_index]
            * mul(vin.position, bone_transforms[vin.bone_indices[bone_index]]);
        blended_normal += vin.bone_weights[bone_index]
            * mul(vin.normal, bone_transforms[vin.bone_indices[bone_index]]);
        blended_tangent += vin.bone_weights[bone_index]
            * mul(vin.tangent, bone_transforms[vin.bone_indices[bone_index]]);
    }
    float4 localPos = float4(blended_position.xyz, 1.0f);
    vin.position = localPos;
    vin.normal = float4(blended_normal.xyz, 0.0f);
    vin.tangent = float4(blended_tangent.xyz, 0.0f);
    
    VS_OUT vout;
    vout.position = mul(localPos, mul(world, view_projection));
    float4 worldPos = mul(localPos, world);
    vout.world_position = worldPos;
    vin.normal.w = 0;
    vout.world_normal = normalize(mul(vin.normal, world));
    vout.world_tangent = normalize(mul(vin.tangent, world));
    vout.world_tangent.w = sigma;
    
    vout.texcoord = vin.texcoord;
    
    // シャドウマップ
    float4 shadow = mul(worldPos, light_view_projection);
    shadow.xyz /= shadow.w;
    shadow.y = -shadow.y;
    shadow.xy = shadow.xy * 0.5f + 0.5f;
    vout.shadow = shadow.xyz;
    
    return vout;
}
