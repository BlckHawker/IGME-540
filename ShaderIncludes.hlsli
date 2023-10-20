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
};

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2
#define MAX_SPECULAR_EXPONENT 256.0f
struct Light
{
    int Type : TYPE; // Which kind of light? 0, 1 or 2 (see above)
    float3 Direction: DIRECTION; // Directional and Spot lights need a direction
    float Range : RANGE; // Point and Spot lights have a max range for attenuation
    float3 Position : POSITION; // Point and Spot lights have a position in space
    float Intensity : INTENSITY; // All lights need an intensity
    float3 Color : COLOR; // All lights need a color
    float SpotFalloff : SPOTFALLOFF; // Spot lights need a value to define their “cone” size
    float3 Padding : PADDING; // Purposefully padding to hit the 16-byte boundary
};

float3 CalculateNormalizedLightDirection(float3 lightDirection)
{
    //Negate the light’s direction, normalize that and store it in another float3 variable
    //You can’t store it back in the light variable because it’s from a constant buffer
    
    return normalize(-lightDirection);
}

float CalculateDiffuseAmount(float3 normal, float3 directionToLight)
{
    //Use the dot(v1, v2) function with the surface’s normal and the direction to the light
    //The surface normal should already be normalized from a previous step
    //The dot product can be negative, which will be problematic if we have multiple lights, so use the saturate() function to clamp the result between 0 and 1 before returning
    return saturate(dot(normal, directionToLight));
}
#endif
