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
#include "Lighting.hlsli"
#include "PerPassData.hlsli"

// Constant data that varies per frame.

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
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

float4 main(VertexOut pin) : SV_Target
{
    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gPerPassData.EyePosW - pin.PosW);

	// Indirect lighting.
    float4 ambient = gLighting.AmbientLight * gDiffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material material = { gDiffuseAlbedo, gFresnelR0, shininess };

    
    float3 directLight = 0.0f;
    uint i = 0;

    uint end = gLighting.NumDirectionalLights;
    for (i = 0; i < end; ++i)
    {
        directLight += ComputeDirectionalLight(gLighting.Lights[i], material, pin.NormalW, toEyeW);
    }
    
    end = end + gLighting.NumPointLights;
    for (i = gLighting.NumDirectionalLights; i < end; ++i)
    {
        directLight += ComputePointLight(gLighting.Lights[i], material, pin.PosW, pin.NormalW, toEyeW);
    }

    end = end + gLighting.NumSpotLights;
    for (i = gLighting.NumDirectionalLights + gLighting.NumPointLights; i < end; ++i)
    {
        directLight += ComputeSpotLight(gLighting.Lights[i], material, pin.PosW, pin.NormalW, toEyeW);
    }

    
    float4 litColor = ambient + float4(directLight, 0.0f);

    // Common convention to take alpha from diffuse material.
    litColor.a = gDiffuseAlbedo.a;

    return litColor;
}