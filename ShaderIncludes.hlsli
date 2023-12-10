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
    float3 tangent : TANGENT;
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
    float3 tangent : TANGENT;
    float4 shadowMapPos : SHADOW_POSITION;
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


static const float MIN_ROUGHNESS = 0.0000001f;
static const float PI = 3.14159265359f;

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
    
    float spec = 0;
    
    if (specExponent > .5)
        spec = pow(saturate(dot(reflectionVector, viewVector)), specExponent);
    
    return spec;
}



float G_SchlickGGX(float3 n, float3 v, float roughness)
{
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));

    return 1 / (NdotV * (1 - k) + k);
}

float D_GGX(float3 n, float3 h, float roughness)
{
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

    return a2 / (PI * denomToSquare * denomToSquare);
}

float3 F_Schlick(float3 v, float3 h, float3 f0)
{
    float VdotH = saturate(dot(v, h));

    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

float CalculatePhongSpecular(float3 cameraPosition, float3 pixelWorldPosition, float3 incomingLightDirection, float3 normal, float roughness)
{
    float3 viewVector = normalize(cameraPosition - pixelWorldPosition);
    float3 reflectionVector = reflect(incomingLightDirection, normal);
    
    return CalcuclateSpecularAmount(reflectionVector, viewVector, roughness);

}

float3 CalculateCookTorrenceSpecular(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
	// Other vectors
    float3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
	// Pass F out of the function for diffuse balance
    F_out = F;

	// Final specular formula
	// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
	// canceled out by our G() term.  As such, they have been removed
	// from BOTH places to prevent floating point rounding errors.
    float3 specularResult = (D * F * G) / 4;

	// One last non-obvious requirement: According to the rendering equation,
	// specular must have the same NdotL applied as diffuse!  We'll apply
	// that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}

float3 CalculateDirectionalLightPhongSpecular(Light directionalLight, float3 normal, float3 cameraPosition, float3 pixelWorldPosition, float roughness, float4 surfaceColor, float specularScale)
{
     //Negate the light’s direction, normalize that and store it in another float3 variable
    float3 lightDirection = normalize(-directionalLight.Direction);
    float3 directionToCamera = normalize(cameraPosition - pixelWorldPosition);
    float diffuseAmount = CalculateDiffuseAmount(normal, lightDirection);
    float phongSpecular = CalculatePhongSpecular(cameraPosition, pixelWorldPosition, -lightDirection, normal, roughness) * specularScale;
    
    phongSpecular *= any(diffuseAmount);
    
    float3 finalColor = surfaceColor.xyz * (diffuseAmount + phongSpecular) * directionalLight.Intensity * directionalLight.Color;
    return finalColor;
}



float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

float3 CalculateDirectionalLightCookTorrenceSpecular(Light light, float3 normal, float3 cameraPosition, float3 pixelWorldPosition, float roughness, float metalness, float3 surfaceColor, float3 f0)
{
    float3 directionToLight = normalize(-light.Direction);
    float3 directionToCamera = normalize(cameraPosition - pixelWorldPosition);
    float diffuseAmount = CalculateDiffuseAmount(normal, directionToLight);
    float3 F;
    float3 specularAmount = CalculateCookTorrenceSpecular(normal, directionToLight, directionToCamera, roughness, f0, F);
    
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    float3 balancedDiff = DiffuseEnergyConserve(diffuseAmount, F, metalness);
    
    // Combine the final diffuse and specular values for this light
    return (balancedDiff * surfaceColor + specularAmount) * light.Intensity * light.Color;
}

float3 CalculatePointLightCookTorrenceSpecular(Light light, float3 normal, float3 cameraPosition, float3 pixelWorldPosition, float roughness, float metalness, float3 surfaceColor, float3 f0)
{
    float3 directionToLight = normalize(light.Position - pixelWorldPosition);
    float3 directionToCamera = normalize(cameraPosition - pixelWorldPosition);
    
    float attenuation = Attenuate(light, pixelWorldPosition);
    float diffuseAmount = CalculateDiffuseAmount(normal, directionToLight);
    float3 F;
    float3 specularAmount = CalculateCookTorrenceSpecular(normal, directionToLight, directionToCamera, roughness, f0, F);
    
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    float3 balancedDiff = DiffuseEnergyConserve(diffuseAmount, F, metalness);
    
    // Combine the final diffuse and specular values for this light
    return (balancedDiff * surfaceColor + specularAmount) * light.Intensity * light.Color * attenuation;
}


float3 CalculatePointLightPhongSpecular(Light pointLight, float3 normal, float3 cameraPosition, float3 pixelWorldPosition, float roughness, float4 surfaceColor, float specularScale)
{
    float3 directionToLight = normalize(pointLight.Position - pixelWorldPosition);
    float3 directiontoCamera = normalize(cameraPosition - pixelWorldPosition);
    
    float attenuation = Attenuate(pointLight, pixelWorldPosition);
    float diffuseAmount = CalculateDiffuseAmount(normal, directionToLight);
    float phongSpecular = CalculatePhongSpecular(directiontoCamera, pixelWorldPosition, directionToLight, normal, roughness) * specularScale;
    
    phongSpecular *= any(diffuseAmount);
    
    float3 finalColor = surfaceColor.xyz * (diffuseAmount + phongSpecular) * pointLight.Intensity * pointLight.Color * attenuation;
    return finalColor;

}

float3 GetLightColorCookTorrenceSpecular(Light light, float3 normal, float3 cameraPosition, float3 worldPosition, float roughness, float metalness, float3 surfaceColor, float3 f0)
{
    switch (light.Type)
    {
        case LIGHT_TYPE_DIRECTIONAL:
            return CalculateDirectionalLightCookTorrenceSpecular(light, normal, cameraPosition, worldPosition, roughness, metalness, surfaceColor, f0);
        default:
            return CalculatePointLightCookTorrenceSpecular(light, normal, cameraPosition, worldPosition, roughness, metalness, surfaceColor, f0);
    }
}

float3 GetLightColorPhongSpecular(Light light, float3 normal, float3 cameraPosition, float3 worldPosition, float roughness, float4 surfaceColor, float specularScale)
{
    switch (light.Type)
    {
        case LIGHT_TYPE_DIRECTIONAL:
            return CalculateDirectionalLightPhongSpecular(light, normal, cameraPosition, worldPosition, roughness, surfaceColor, specularScale);
        
        default:
            return CalculatePointLightPhongSpecular(light, normal, cameraPosition, worldPosition, roughness, surfaceColor, specularScale);
    }
}



#endif
