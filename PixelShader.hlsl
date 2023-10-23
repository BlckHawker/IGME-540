
#include "ShaderIncludes.hlsli"
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float roughness;
    float3 cameraPosition;
    float3 ambient;
    Light lights[5];  
}

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
	
    float3 smallAmbience = ambient / 5;
    input.normal = normalize(input.normal);  
    
    float3 lightSum = float3(0, 0, 0);
    
    for (int i = 0; i < 5; i++)
    {
        lightSum += GetLightColor(lights[i], input.normal, cameraPosition, input.worldPosition, roughness, colorTint);

    }
    
    return float4(lightSum, 1);
}