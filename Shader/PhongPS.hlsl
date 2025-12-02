#include "Phong.hlsli"

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
SamplerState linearSampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = diffuseMap.Sample(linearSampler, pin.texcoord) * materialColor;
    
    float3 N = normalize(pin.normal);
    float3 T = normalize(pin.tangent);
    float3 B = normalize(cross(N, T)); // Binormal(従法線)はNormal(法線)と Tangent(接線) との外積で算出できる
    
    // ノーマルマップ
    float3 normal = normalMap.Sample(linearSampler, pin.texcoord).xyz;
    normal = (normal * 2.0f) - 1.0f;
    N = normalize((normal.x * T) + (normal.y * B) + (normal.z * N));
    
    float3 L = normalize(-lightDirection.xyz);
    float power = max(0, dot(L, N));
    
    // フランバートシェーディング
    // 陰影をつける強度を 70%にして 30%は光が当たっているようにする
    power = power * 0.7 + 0.3f;
    
    // ライト適用
    color.rgb *= lightColor.rgb * power;
    
    // スペルキュラ
    float3 V = normalize(cameraPosition.xyz - pin.position);
    float3 specular = pow(max(0, dot(N, normalize(V + L))), 128);
    color.rgb += specular;
    
    return color;
}