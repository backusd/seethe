// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "InstanceData.hlsli"
#include "Lighting.hlsli"
#include "PerPassData.hlsli"

cbuffer cbInstanceData : register(b0)
{
    InstanceData gInstanceDataArray[MAX_INSTANCES];
}

cbuffer cbMaterial : register(b1)
{
    Material gMaterial[NUM_MATERIALS];
};

cbuffer cbPass : register(b2)
{
    PerPassData gPerPassData;
    SceneLighting gLighting;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    nointerpolation uint MaterialIndex : MATERIAL_INDEX;
};


VertexOut main(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    //float4x4 world = gInstanceWorldArray[instanceID];
    float4x4 world = gInstanceDataArray[instanceID].World;
    
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3) world);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gPerPassData.ViewProj);
    
    //vout.MaterialIndex = vin.MaterialIndex;
    vout.MaterialIndex = gInstanceDataArray[instanceID].MaterialIndex;

    return vout;
}