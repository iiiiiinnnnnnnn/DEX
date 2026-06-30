// Default model shader

#include "Model.hlsli"

cbuffer Material : register(b4)
{
    float4 diffuseColor;
    float specularPower;
    float3 __dummy;
};

Texture2D shadowMap : register(t0);
Texture2D diffuseMap : register(t1);
Texture2D normalMap : register(t2);

SamplerState linearSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

float4 main(VS_OUT pin) : SV_TARGET
{
    // ベースカラー
    float4 texcolor = diffuseMap.Sample(linearSampler, pin.texcoord) * diffuseColor;
    float alpha = texcolor.a;
    
#if 0
    // Inverse gamma process
    const float GAMMA = 2.2;
    color.rgb = pow(color.rgb, GAMMA);
#endif
    
    float3 N_base = normalize(pin.world_normal.xyz);
    float3 T_base = normalize(pin.world_tangent.xyz);
    T_base = normalize(T_base - N_base * dot(N_base, T_base));
    float3 B_base = normalize(cross(N_base, T_base) * pin.world_tangent.w);
    float3 tsNormal = normalMap.Sample(linearSampler, pin.texcoord).xyz;
    tsNormal = tsNormal * 2.0 - 1.0;
    float3 worldNormal = normalize(tsNormal.x * T_base + tsNormal.y * B_base + tsNormal.z * N_base);
    float3 N = worldNormal;
    float3 T = normalize(pin.world_tangent.xyz);
    float sigma = pin.world_tangent.w;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);
    
    float3 normal = tsNormal;
    normal.y = -normal.y;
    
    N = normalize((normal.x * T) + (normal.y * B) + (normal.z * N));
        
    float3 L = normalize(-directionLight.direction.xyz);

    float3 diffuse = texcolor.rgb * max(0, dot(N, L));
    float3 V = normalize(camera_position.xyz - pin.world_position.xyz);
    float specular = pow(max(0, dot(N, normalize(V + L))), 128) * specularPower;
    
    float3 ambient = texcolor.rgb * 0.2f;
    float3 light = ambient + (diffuse + specular) * directionLight.color.rgb * directionLight.intensity;
    
    // シャドウ
    const float shadowBias = 0.001f;
    float shadowFactor = 0.0f;
    const float2 shadowTexelOffsets[9] =
    {
        float2(-shadowTexelSize, -shadowTexelSize), // 左上
        float2(0.0f, -shadowTexelSize), // 上
        float2(shadowTexelSize, -shadowTexelSize), // 右上
        float2(-shadowTexelSize, 0.0f), // 左
        float2(0.0f, 0.0f), // 中
        float2(shadowTexelSize, 0.0f), // 右
        float2(-shadowTexelSize, shadowTexelSize), // 左下
        float2(0.0f, shadowTexelSize), // 下
        float2(shadowTexelSize, shadowTexelSize) // 右下
    };
    [unroll]
    for (int l = 0; l < 9; ++l)
    {
        shadowFactor += shadowMap.SampleCmpLevelZero(
            shadowSampler, pin.shadow.xy + shadowTexelOffsets[l], pin.shadow.z - shadowBias).r;
    }
    shadowFactor /= 9.0f;
    float3 shadow = lerp(float3(0, 0, 0), float3(1.0f, 1.0f, 1.0f), shadowFactor);
    shadow = pow(shadow, 2.0f);
    light *= shadow;

    float4 finalcolor = float4(light, alpha);
    
    //========================================
    // ポイントライト 10 個
    //========================================
    for (int i = 0; i < 10; ++i)
    {
        float3 LP = pin.world_position.xyz - pointLight[i].position.xyz;
        float len = length(LP);
        if (len >= pointLight[i].range)
            continue;

        float attenuateLength = saturate(1.0f - len / pointLight[i].range);
        float attenuation = attenuateLength * attenuateLength;
        LP /= len;

        float diff = max(0, dot(N, -LP));

        float3 halfV = normalize(V - LP);
        float spec = pow(max(0, dot(N, halfV)), 128) * specularPower;
        
        float3 pdiff = texcolor.rgb * pointLight[i].color.rgb * diff * attenuation;
        float3 pspec = pointLight[i].color.rgb * spec * attenuation;

        finalcolor.rgb += (pdiff + pspec) * pointLight[i].intensity;
    }
    
    //========================================
    // スポットライト 10 個
    //========================================
    for (int j = 0; j < 10; ++j)
    {
        float3 LP = pin.world_position.xyz - spotLight[j].position.xyz;
        float len = length(LP);
        if (len >= spotLight[j].range)
            continue;

        float attenuateLength = saturate(1.0f - len / spotLight[j].range);
        float attenuation = attenuateLength * attenuateLength;
        LP /= len;

        float3 spotDir = normalize(spotLight[j].direction.xyz);
        float angle = dot(spotDir, LP);
        float area = spotLight[j].innerCorn - spotLight[j].outerCorn;
        attenuation *= saturate(1.0f - (spotLight[j].innerCorn - angle) / area);

        float diff = max(0, dot(N, -LP));

        float3 halfV = normalize(V - LP);
        float spec = pow(max(0, dot(N, halfV)), 128) * specularPower;
        
        float3 sdiff = texcolor.rgb * spotLight[j].color.rgb * diff * attenuation;
        float3 sspec = spotLight[j].color.rgb * spec * attenuation;

        finalcolor.rgb += (sdiff + sspec) * spotLight[j].intensity;
    }
    
    return finalcolor;
}
