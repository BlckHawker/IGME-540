struct SkyboxVertexShaderInput
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

struct SkyboxVertexToPixel
{
    float4 position : SV_POSITION;
    float3 sampleDir : DIRECTION;
};