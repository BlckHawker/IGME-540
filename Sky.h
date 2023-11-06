#pragma once

#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <memory>
#include <wrl/client.h>
#include "WICTextureLoader.h"

using namespace Microsoft::WRL;
using namespace std;

class Sky
{
public:
	Sky(const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back,
		shared_ptr<Mesh> mesh, 
		shared_ptr<SimpleVertexShader> vertexShader,
		shared_ptr<SimplePixelShader> pixelShader,
		ComPtr<ID3D11SamplerState> samplerState,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	~Sky();

	void Draw(std::shared_ptr<Camera> camera);

private:
	ComPtr<ID3D11SamplerState> samplerState; //for sampler options
	ComPtr<ID3D11ShaderResourceView> cubeSRV; // for the cube map texture’s SRV
	ComPtr<ID3D11DepthStencilState> depthState; // for adjusting the depth buffer comparison type
	ComPtr<ID3D11RasterizerState> rasterizerState;// for rasterizer options(drawing the object’s “inside”)
	shared_ptr<Mesh> mesh; //for the geometry to use when drawing the sky
	shared_ptr<SimplePixelShader> pixelShader; //for the sky - specific pixel shader
	shared_ptr<SimpleVertexShader> vertexShader; //for the sky - specific vertex shader
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
};

