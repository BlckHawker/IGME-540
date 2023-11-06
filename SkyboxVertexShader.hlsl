#include "SkyboxShaderStruct.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix viewMatrix, projectionMatrix;
}

SkyboxVertexToPixel main(SkyboxVertexShaderInput input)
{
    SkyboxVertexToPixel output;
    
    matrix viewMatrixNoTranslation = viewMatrix;
    
    viewMatrixNoTranslation._14 = 0;
    viewMatrixNoTranslation._24 = 0;
    viewMatrixNoTranslation._34 = 0;
    
    output.position = mul(mul(projectionMatrix, viewMatrixNoTranslation), float4(input.localPosition, 1.0f));
    
    //want to be on the end of the far clop plane
    output.position.z = output.position.w;
    
    //direction of the skybox on the map
    output.sampleDir = input.localPosition;
    
    return output;

}