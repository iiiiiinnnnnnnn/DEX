#include "Phong.hlsli"

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D shadowMap : register(t2);
SamplerState linearSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

float4 main(VS_OUT pin) : SV_TARGET
{
    // ベースカラー
    float4 color = diffuseMap.Sample(linearSampler, pin.texcoord) * materialColor;
    
    // ノーマルマップ
    float3 N = normalize(pin.normal);
    float3 T = normalize(pin.tangent);
    float3 B = normalize(cross(N, T)); // Binormal(従法線)はNormal(法線)と Tangent(接線) との外積で算出できる
    float3 normal = normalMap.Sample(linearSampler, pin.texcoord).xyz;
    normal = (normal * 2.0f) - 1.0f;
    N = normalize((normal.x * T) + (normal.y * B) + (normal.z * N));
    
    float3 L = normalize(-directionLight.direction.xyz);
    float power = max(0, dot(L, N));
    
    // フランバートシェーディング
    // 陰影をつける強度を 70%にして 30%は光が当たっているようにする
    power = power * 0.7 + 0.3f;
    
    // ライト
    color.rgb *= directionLight.color.rgb * power;
    
    // スペルキュラ
    float3 V = normalize(cameraPosition.xyz - pin.position);
    float3 specular = pow(max(0, dot(N, normalize(V + L))), 128);
    color.rgb += specular;
    
    
    //========================================
    // ポイントライト 10 個
    //========================================
    for (int i = 0; i < 10; ++i)
    {
        float3 LP = pin.position.xyz - pointLight[i].position.xyz;
        float len = length(LP);
        if (len >= pointLight[i].range)
            continue;

        float attenuateLength = saturate(1.0f - len / pointLight[i].range);
        float attenuation = attenuateLength * attenuateLength;
        LP /= len;

        float diff = max(0, dot(N, -LP));
        float3 pdiff = pointLight[i].color.rgb * diff * attenuation;

        float3 halfV = normalize(V - LP);
        float spec = pow(max(0, dot(N, halfV)), 128);
        float3 pspec = pointLight[i].color.rgb * spec * attenuation;

        color.rgb += pdiff + pspec;
    }


    //========================================
    // スポットライト 10 個
    //========================================
    for (int j = 0; j < 10; ++j)
    {
        float3 LP = pin.position.xyz - spotLight[j].position.xyz;
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
        float3 sdiff = spotLight[j].color.rgb * diff * attenuation;

        float3 halfV = normalize(V - LP);
        float spec = pow(max(0, dot(N, halfV)), 128);
        float3 sspec = spotLight[j].color.rgb * spec * attenuation;

        color.rgb += sdiff + sspec;
    }
    
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
    for (int i = 0; i < 9; ++i)
    {
        shadowFactor += shadowMap.SampleCmpLevelZero(shadowSampler, pin.shadow.xy + shadowTexelOffsets[i],
            pin.shadow.z - shadowBias).r;
    }
    shadowFactor /= 9.0f;
    float3 shadow = lerp(shadowColor.rgb, float3(1.0f, 1.0f, 1.0f), shadowFactor);
    color.rgb *= shadow;
    
    // アンビエント
    color.rgb += ambientColor.rgb;
    
    return color;
}