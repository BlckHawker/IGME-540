#include "ShaderIncludes.hlsli"
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float roughness;
    float3 cameraPosition;
    float3 ambient;
    Light lights[2];  
    float2 uvOffset;
}

Texture2D SurfaceTexture : register(t0); // "t" registers for textures
Texture2D SpecularMap : register(t1);
Texture2D NormalMap : register(t2);
SamplerState BasicSampler : register(s0); // "s" registers for samplers

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
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
    // Adjust the variables below as necessary to work with your own code
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);

    // Feel free to adjust/simplify this code to fit with your existing shader(s)
    // Simplifications include not re-normalizing the same vector more than once!
    float3 N = input.normal; // Must be normalized here or before
    float3 T = input.tangent; // Must be normalized here or before
    T = normalize(T - N * dot(T, N)); // Gram-Schmidt assumes T&N are normalized!
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);
    
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2.0f - 1.0f;
    unpackedNormal = normalize(unpackedNormal);
    
    // Assumes that input.normal is the normal later in the shader
    input.normal = mul(unpackedNormal, TBN); // Note multiplication order!
    
    float3 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv).rgb * colorTint.rgb;
    float specularScale = SpecularMap.Sample(BasicSampler, input.uv).b;
    
    float3 lightSum = surfaceColor * ambient;
    
    for (int i = 0; i < 2; i++)
    {
        lightSum += GetLightColor(lights[i], input.normal, cameraPosition, input.worldPosition, roughness, float4(surfaceColor, 1.0f), specularScale);
    }
    
    return float4(lightSum, 1.0f);
}