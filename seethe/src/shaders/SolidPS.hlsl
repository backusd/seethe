// Include structures and functions for lighting.
#include "Lighting.hlsli"
#include "PerPassData.hlsli"


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbPass : register(b1)
{
    PerPassData gPerPassData;
};

cbuffer cbLighting : register(b2)
{
    SceneLighting gLighting;
};

cbuffer cbMaterial : register(b3)
{
    Material gMaterial[NUM_MATERIALS];
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