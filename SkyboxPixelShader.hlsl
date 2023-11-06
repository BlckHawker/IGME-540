#include "SkyboxShaderStruct.hlsli"

TextureCube SkyTexture : register(t0);
SamplerState BasicSampler : register(s0);


float4 main(SkyboxVertexToPixel input) : SV_TARGET
{
    return SkyTexture.Sample(BasicSampler, input.sampleDir);
}