#include "ShaderIncludes.hlsli"

#define MAX_LIGHTS 128
static const float F0_NON_METAL = 0.04f;

cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float roughness;
    float3 cameraPosition;
    float3 ambient;
    Light lights[MAX_LIGHTS];
    int lightNum;
    float2 uvOffset;
    int useGammaCorrection;
}

Texture2D AlbedoMap : register(t0); // "t" registers for textures
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);
Texture2D ShadowMap : register(t4);
SamplerState BasicSampler : register(s0); // "s" registers for samplers
SamplerComparisonState ShadowSampler : register(s1);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    float3 N = input.normal; // Must be normalized here or before
    float3 T = input.tangent; // Must be normalized here or before
    T = normalize(T - N * dot(T, N)); // Gram-Schmidt assumes T&N are normalized!
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2.0f - 1.0f;
    unpackedNormal = normalize(unpackedNormal);
    input.normal = mul(unpackedNormal, TBN); 
    
    float3 surfaceColor = AlbedoMap.Sample(BasicSampler, input.uv).rgb;

    //uncorrect the gamma from the texture if using gammaCorrect
    surfaceColor = useGammaCorrection ? pow(surfaceColor, 2.2f) : surfaceColor;
    
    surfaceColor *= colorTint.rgb;
    
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;
    
    // Assume albedo texture is actually holding specular color where metalness == 1
    // Note the use of lerp here - metal is generally 0 or 1, but might be in between
    // because of linear texture sampling, so we lerp the specular color to match
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    
    // Perform the perspective divide (divide by W) ourselves
    input.shadowMapPos /= input.shadowMapPos.w;
    // Convert the normalized device coordinates to UVs for sampling
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    // Grab the distances we need: light-to-pixel and closest-surface
    float distToLight = input.shadowMapPos.z;
    
    float shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler,shadowUV,distToLight).r;
    
    float3 lightSum = float3(0,0,0);
   
    int lightUsed = lightNum > MAX_LIGHTS ? MAX_LIGHTS : lightNum;
    
    for (int i = 0; i < lightUsed; i++)
    {
        float3 lightResult = GetLightColorCookTorrenceSpecular(lights[i], input.normal, cameraPosition, input.worldPosition, roughness, metalness, surfaceColor, specularColor);
        
        lightSum += i == 0 ? (lightResult * shadowAmount) : lightResult;
    }
    
    //aply gamma correction
    lightSum = useGammaCorrection ? pow(lightSum, 1.0f / 2.2f) : lightSum;
    
    return float4(lightSum, 1.0f);
}