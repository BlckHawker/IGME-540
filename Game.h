#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>
#include <memory>
#include "Mesh.h"
#include "Entity.h"
#include "Camera.h"

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

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();
	void CameraInput(float dt);

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	//used to edit shader
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer;

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	//meshes to draw
	std::vector<std::shared_ptr<Mesh>> meshes;

	//value for translalting mesh
	float rotationValue = 0;
	bool automaticTranslation = true;
	bool automaticRotation = true;
	bool automaticScaling = true;

	const int entityNum = 5; //the amount of entities that will spawn
	std::vector<Entity> entities;

	int activeCameraIndex = 1;
	std::vector< std::shared_ptr<Camera>> cameras;
};

