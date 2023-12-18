// Include structures and functions for lighting.
#include "InstanceData.hlsli"
#include "Lighting.hlsli"
#include "PerPassData.hlsli"


cbuffer cbInstanceData : register(b0)
{
    InstanceData gInstanceDataArray[MAX_INSTANCES];
}

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

struct VertexIn
{
    float4 PosL : POSITION;
//    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut main(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    float4x4 world = gInstanceDataArray[instanceID].World;
    float4 posW = mul(vin.PosL, world);
    vout.PosH = mul(posW, gPerPassData.ViewProj);
	
	// Just pass vertex color into the pixel shader.
//    vout.Color = vin.Color;
    
    // When rendering as a solid color, we use the diffuse albedo as the color
    uint matIndex = gInstanceDataArray[instanceID].MaterialIndex;
    vout.Color = gMaterial[matIndex].DiffuseAlbedo;
    return vout;
}