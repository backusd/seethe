
// Max constant buffer size is 4096 float4's
// Our current SceneLighting class uses 2 float4's for non-Light data
// And the Light struct is 3 float4's
// (4096 - 2) / 3 = 1364.67
#define MAX_LIGHTS 1364

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only
};

struct SceneLighting
{
    float4 AmbientLight;
    uint NumDirectionalLights;
    uint NumPointLights;
    uint NumSpotLights;
    uint Pad0;
    
    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MAX_LIGHTS per object.
    Light Lights[MAX_LIGHTS];
};

#define NUM_MATERIALS 10

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);
    
    float3 result = float3(0.0f, 0.0f, 0.0f);

    // Range test.
    if (d < L.FalloffEnd)
    {
        // Normalize the light vector.
        lightVec /= d;

        // Scale light down by Lambert's cosine law.
        float ndotl = max(dot(lightVec, normal), 0.0f);
        float3 lightStrength = L.Strength * ndotl;

        // Attenuate light by distance.
        float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
        lightStrength *= att;

        result = BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
    }
    
    return result;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);
    
    float3 result = float3(0.0f, 0.0f, 0.0f);

    // Range test.
    if (d < L.FalloffEnd)
    {
        // Normalize the light vector.
        lightVec /= d;

        // Scale light down by Lambert's cosine law.
        float ndotl = max(dot(lightVec, normal), 0.0f);
        float3 lightStrength = L.Strength * ndotl;

        // Attenuate light by distance.
        float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
        lightStrength *= att;

        // Scale by spotlight
        float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
        lightStrength *= spotFactor;

        result = BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
    }
    
    return result;
}


