//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

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

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    uint   MaterialIndex : MATERIAL_INDEX;
};

float4 main(VertexOut pin) : SV_Target
{
    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gPerPassData.EyePosW - pin.PosW);
    
    // One the CPU side, we define Material with a roughness value which initially gets assigned to the
    // Shininess value on the GPU side. Therefore, we take 1 minus that value to compute the shininess
    Material material = gMaterial[pin.MaterialIndex];
    material.Shininess = 1.0f - material.Shininess;

	// Indirect lighting.
    float4 ambient = gLighting.AmbientLight * material.DiffuseAlbedo;
    
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLighting.Lights, material, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse material.
    litColor.a = material.DiffuseAlbedo.a;

    return litColor;
}