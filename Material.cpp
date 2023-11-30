#include "Material.h"



Material::Material(DirectX::XMFLOAT4 colorTint, std::shared_ptr<SimplePixelShader> pixelShader, std::shared_ptr<SimpleVertexShader> vertexShader) :
	colorTint(colorTint),
	pixelShader(pixelShader),
	vertexShader(vertexShader)
{

}

DirectX::XMFLOAT4 Material::GetColorTint()
{
	return colorTint;
}

std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
	return pixelShader;
}

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader()
{
	return vertexShader;
}

void Material::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
	this->colorTint = colorTint;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
{
	this->pixelShader = pixelShader;
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader)
{
	this->vertexShader = vertexShader;
}

void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV)
{
	textureSRVs.insert({ name, textureSRV });
}

void Material::AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
	samplers.insert({ name, sampler });
}

float Material::Clamp(float val)
{
	return val < 0.0f ? 0.0f : val > 1.0f ? 1.0f : val;
}

void Material::SetLights(std::string name, const void* data, unsigned int size)
{
	pixelShader->SetData(name, data, size);
}

void Material::SetTextureData()
{
	for (auto& t : textureSRVs) { pixelShader->SetShaderResourceView(t.first.c_str(), t.second); }
	for (auto& s : samplers) { pixelShader->SetSamplerState(s.first.c_str(), s.second.Get()); }
}