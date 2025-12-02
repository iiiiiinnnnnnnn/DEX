struct VS_OUT
{
    float4 vertex : SV_POSITION;
};
cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
};