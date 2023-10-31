
#include "ShaderIncludes.hlsli"
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float roughness;
    float3 cameraPosition;
    float3 ambient;
    Light lights[5];  
    float2 uvOffset;
}

Texture2D SurfaceTexture : register(t0); // "t" registers for textures
Texture2D SpecularMap : register(t1);
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
    float3 surfaceColor = SurfaceTexture.Sample(BasicSampler, input.uv).rgb * colorTint.rgb;
    float specularScale = SpecularMap.Sample(BasicSampler, input.uv).b;
    
    float3 smallAmbience = ambient / 5;
    input.normal = normalize(input.normal);  
    
    float3 lightSum = float3(0, 0, 0);
    
    for (int i = 0; i < 5; i++)
    {
        lightSum += GetLightColor(lights[i], input.normal, cameraPosition, input.worldPosition, roughness, float4(surfaceColor, 1), specularScale);

    }
    
    return float4(lightSum, 1);
}