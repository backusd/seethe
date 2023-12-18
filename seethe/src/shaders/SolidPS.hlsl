// Include structures and functions for lighting.
#include "Lighting.hlsli"
#include "PerPassData.hlsli"


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
    Material gMaterial[NUM_MATERIALS];
};

cbuffer cbPass : register(b2)
{
    PerPassData gPerPassData;
    SceneLighting gLighting;
};




struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(VertexOut pin) : SV_Target
{
    return pin.Color;
}