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

// Include structures and functions for lighting.
#include "LightingUtil.hlsli"

// Constant data that varies per frame.

#define MAX_INSTANCES 100
#define NUM_MATERIALS 10

struct MaterialIn
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float  Roughness;
};

struct InstanceData
{
    float4x4 World;
    uint MaterialIndex;
    uint Pad0;
    uint Pad1;
    uint Pad2;
};

cbuffer cbInstanceData : register(b0)
{
    InstanceData gInstanceDataArray[MAX_INSTANCES];
}

cbuffer cbMaterial : register(b1)
{
    MaterialIn gMaterial;
};

cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
//    uint   MaterialIndex : MATERIAL_INDEX;
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
    vout.PosH = mul(posW, gViewProj);
    
    //vout.MaterialIndex = vin.MaterialIndex;
    vout.MaterialIndex = gInstanceDataArray[instanceID].MaterialIndex;

    return vout;
}