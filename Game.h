#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "Entity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Material.h"
#include "Lights.h"
#include "WICTextureLoader.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Mesh.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "Sky.h"
#include <vector>
#include <memory>

using namespace DirectX;

class Game : public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:
	//helper method for igmu
	void ImGuiInitialization(float deltaTime, unsigned int windowHeight, unsigned int windowWidth);
	void CreateMaterials();
	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateEntites();
	void CameraInput(float dt);
	void LoadAssets();
	void CreateLights();
	void CreateShadowMapResources();
	void RenderShadowMap();
	void BloomPostProcessing();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Shaders and shader-related constructs
	std::vector<std::shared_ptr<SimpleVertexShader>> vertexShaders;
	std::vector<std::shared_ptr<SimplePixelShader>> pixelShaders;
	std::vector<std::shared_ptr<SimpleVertexShader>> skyBoxVertexShaders;
	std::vector<std::shared_ptr<SimplePixelShader>> skyBoxPixelShaders;


	std::vector<std::shared_ptr<Mesh>> meshes;

	std::vector<std::shared_ptr<Material>> materials;

	std::shared_ptr<Sky> skyBox;

	const int entityNum = 9; //the amount of entities that will spawn
	std::vector<shared_ptr<Entity>> entities;

	int activeCameraIndex = 1;
	std::vector< std::shared_ptr<Camera>> cameras;
	std::vector<Light> lights;

	bool rotate = true; //tells emttites to rotate

	shared_ptr<Material> floorMaterial;
	std::shared_ptr<Entity> floorEntity;


	//shadow map stuff
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;
	int shadowMapResolution = 1024; // Ideally a power of 2 (like 1024)


	// Resources that are shared among all post processes
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppSampler;
	std::shared_ptr<SimpleVertexShader> ppVS;

	// Resources that are tied to a particular post process
	std::shared_ptr<SimplePixelShader> ppPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppRTV; // For rendering
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppSRV; // For sampling

	float blurAmount = 0;




};

