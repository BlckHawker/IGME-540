#include <DirectXMath.h>
#include <memory>
#include "SimpleShader.h"
#pragma once
class Material
{
private:
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	float roughness;
	float Clamp(float val);
public:
	Material(float roughness, DirectX::XMFLOAT4 colorTint, 
		std::shared_ptr<SimplePixelShader> pixelShader,
		std::shared_ptr<SimpleVertexShader> vertexShader);

	DirectX::XMFLOAT4 GetColorTint();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	float GetRoughness();

	void SetRoughness(float roughness);
	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
};

