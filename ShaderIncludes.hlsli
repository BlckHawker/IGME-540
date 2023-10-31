#ifndef __GGP_SHADER_INCLUDES__ // Each .hlsli file needs a unique identifier!
#define __GGP_SHADER_INCLUDES__ 
// ALL of your code pieces (structs, functions, etc.) go here!
// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float3 localPosition : POSITION; // XYZ position
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
	
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPosition : POSITION;
};

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2
#define MAX_SPECULAR_EXPONENT 256.0f
struct Light
{
    int Type : TYPE; // Which kind of light? 0, 1 or 2 (see above)
    float3 Direction : DIRECTION; // Directional and Spot lights need a direction
    float Range : RANGE; // Point and Spot lights have a max range for attenuation
    float3 Position : POSITION; // Point and Spot lights have a position in space
    float Intensity : INTENSITY; // All lights need an intensity
    float3 Color : COLOR; // All lights need a color
    float SpotFalloff : SPOTFALLOFF; // Spot lights need a value to define their “cone” size
    float3 Padding : PADDING; // Purposefully padding to hit the 16-byte boundary
};

float CalculateDiffuseAmount(float3 normal, float3 directionToLight)
{
    //Use the dot(v1, v2) function with the surface’s normal and the direction to the light
    //The surface normal should already be normalized from a previous step
    //The dot product can be negative, which will be problematic if we have multiple lights, so use the saturate() function to clamp the result between 0 and 1 before returning
    return saturate(dot(normal, directionToLight));
}

float CalcuclateSpecularAmount(float3 reflectionVector, float3 viewVector, float roughness)
{
    float specExponent = (1.0 - roughness) * MAX_SPECULAR_EXPONENT;
    
    if (specExponent < .5)
        return 0;
    
    return pow(saturate(dot(reflectionVector, viewVector)), specExponent);
}

float CalculatePhongSpecular(float3 cameraPosition, float3 pixelWorldPosition, float3 incomingLightDirection, float3 normal, float roughness)
{
    float3 viewVector = normalize(cameraPosition - pixelWorldPosition);
    float3 reflectionVector = reflect(incomingLightDirection, normal);
    
    return CalcuclateSpecularAmount(reflectionVector, viewVector, roughness);

}

float3 CalculateDirectionalLight(Light directionalLight, float3 normal, float3 cameraPosition, float3 pixelWorldPosition, float roughness, float4 surfaceColor, float specularScale)
{
     //Negate the light’s direction, normalize that and store it in another float3 variable
    float3 lightDirection = normalize(-directionalLight.Direction);
    float3 directionToCamera = normalize(cameraPosition - pixelWorldPosition);
    float diffuseAmount = CalculateDiffuseAmount(normal, lightDirection);
    float phongSpecular = CalculatePhongSpecular(cameraPosition, pixelWorldPosition, -lightDirection, normal, roughness) * specularScale;
    float3 finalColor = surfaceColor.xyz * (diffuseAmount + phongSpecular) * directionalLight.Intensity * directionalLight.Color;
    return finalColor;
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}


float3 CalculatePointLight(Light pointLight, float3 normal, float3 cameraPosition, float3 pixelWorldPosition, float roughness, float4 surfaceColor, float specularScale)
{
    float3 directionToLight = normalize(pointLight.Position - pixelWorldPosition);
    float3 directiontoCamera = normalize(cameraPosition - pixelWorldPosition);
    
    float attenuation = Attenuate(pointLight, pixelWorldPosition);
    float diffuseAmount = CalculateDiffuseAmount(normal, directionToLight);
    float phongSpecular = CalculatePhongSpecular(directiontoCamera, pixelWorldPosition, directionToLight, normal, roughness) * specularScale;
    float3 finalColor = surfaceColor.xyz * (diffuseAmount + phongSpecular) * pointLight.Intensity * pointLight.Color;
    return finalColor;

}

float3 GetLightColor(Light light, float3 normal, float3 cameraPosition, float3 worldPosition, float roughness, float4 surfaceColor, float specularScale)
{
    switch (light.Type)
    {
        case LIGHT_TYPE_DIRECTIONAL:
            return CalculateDirectionalLight(light, normal, cameraPosition, worldPosition, roughness, surfaceColor, specularScale);
        
        default:
            return CalculatePointLight(light, normal, cameraPosition, worldPosition, roughness, surfaceColor, specularScale);
    }
}



#endif
