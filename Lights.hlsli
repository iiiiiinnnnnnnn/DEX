// ディレクションライト
struct DirectionLight
{
    float3 direction;
    float intensity;
    float4 color;
};

// ポイントライト
struct PointLight
{
    float3 position;
    float range;
    float4 color;
    float intensity;
    float3 _dummy;
};

// スポットライト
struct SpotLight
{
    float3 position;
    float range;
    float3 direction;
    float innerCorn;
    float4 color;
    float outerCorn;
    float intensity;
    float _dummy2;
    float _dummy3;
};
