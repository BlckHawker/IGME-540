#include "Material.h"



Material::Material(float roughness, DirectX::XMFLOAT4 colorTint, std::shared_ptr<SimplePixelShader> pixelShader, std::shared_ptr<SimpleVertexShader> vertexShader) :
	colorTint(colorTint),
	pixelShader(pixelShader),
	vertexShader(vertexShader)
{
	SetRoughness(roughness);
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

float Material::GetRoughness()
{
	return roughness;
}

void Material::SetRoughness(float roughness)
{
	this->roughness = Clamp(roughness);
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

float Material::Clamp(float val)
{
	return val < 0 ? 0 : val > 1 ? 1 : val;
}