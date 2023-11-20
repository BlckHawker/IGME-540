#include "Game.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	float aspectRatio = (float)this->windowWidth / this->windowHeight;

	for (int i = 0; i < 3; i++)
	{
		cameras.push_back(std::make_shared<Camera>(aspectRatio));

		float xPos;
		float yRotation;
		float fov;

		switch (i)
		{
			case 0:
				xPos = -1.0f;
				yRotation = DirectX::XM_PIDIV4;
				fov = DirectX::XM_PIDIV4;
				break;
			case 2:
				xPos = 1.0f;
				yRotation = -DirectX::XM_PIDIV4;
				fov = DirectX::XM_PI / 3.0f;
				break;
		}

		if (i != 1)
		{
			cameras[i]->GetTransform()->MoveAbsolute(xPos, 0.0f, 0.0f);
			cameras[i]->GetTransform()->Rotate(0.0f, yRotation, 0.0f);
			cameras[i]->SetFieldOfView(fov, aspectRatio);
		}

	}

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();
	// 
	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	LoadAssets();

	// Tell the input assembler (IA) stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our vertices?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}



void Game::LoadAssets()
{

	//sampler states
	D3D11_SAMPLER_DESC samplerData = {};
	samplerData.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerData.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerData.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerData.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerData.MaxAnisotropy = 16;
	samplerData.MaxLOD = D3D11_FLOAT32_MAX;

	// Quick pre-processor macro for simplifying texture loading calls below
	#define LoadTexture(path, srv) CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(path).c_str(), 0, srv.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	HRESULT a1 = device->CreateSamplerState(&samplerData, samplerState.GetAddressOf());

	//create skybox
	
	std::shared_ptr<Mesh> skybBoxMesh = std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/cube.obj").c_str());
	std::shared_ptr<SimpleVertexShader> skyBoxVertexShaders = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"SkyboxVertexShader.cso").c_str());
	std::shared_ptr<SimplePixelShader> skyBoxPixelShaders = std::make_shared<SimplePixelShader>(device, context, FixPath(L"SkyboxPixelShader.cso").c_str());

	skyBox = std::make_shared<Sky>(
		FixPath(L"../../Assets/SkyBoxes/Clouds Pink/right.png").c_str(),
		FixPath(L"../../Assets/SkyBoxes/Clouds Pink/left.png").c_str(),
		FixPath(L"../../Assets/SkyBoxes/Clouds Pink/up.png").c_str(),
		FixPath(L"../../Assets/SkyBoxes/Clouds Pink/down.png").c_str(),
		FixPath(L"../../Assets/SkyBoxes/Clouds Pink/front.png").c_str(),
		FixPath(L"../../Assets/SkyBoxes/Clouds Pink/back.png").c_str(),
		skybBoxMesh,
		skyBoxVertexShaders,
		skyBoxPixelShaders,
		samplerState,
		device,
		context);

	//Load the meshes
	meshes.push_back(std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/cube.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/cylinder.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/sphere.obj").c_str()));

	//Load the textures

	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> srvAlbedoMapVector, srvNormalMapVector, srvRoughnessMapVector, srvMetalMapVector;

	//Shader Resource View
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeAlbedoSRV, cobblestoneAlbedoSRV, scratchedAlbedoSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormalSRV, cobblestoneNormalSRV, scratchedNormalSRV, flatNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughnessSRV, cobblestoneRoughnessSRV, scratchedRoughnessSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeMetalSRV, cobblestoneMetalSRV, scratchedMetalSRV;


	//================================================================================

	LoadTexture(L"../../Assets/Textures/Albedo Maps/bronze.png", bronzeAlbedoSRV);
	LoadTexture(L"../../Assets/Textures/Albedo Maps/cobblestone.png", cobblestoneAlbedoSRV);
	LoadTexture(L"../../Assets/Textures/Albedo Maps/scratched.png", scratchedAlbedoSRV);

	LoadTexture(L"../../Assets/Textures/Normal Maps/bronze.png", bronzeNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/cobblestone.png", cobblestoneNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/scratched.png", scratchedNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/flat.png", scratchedNormalSRV);

	LoadTexture(L"../../Assets/Textures/Roughness Maps/bronze.png", bronzeRoughnessSRV);
	LoadTexture(L"../../Assets/Textures/Roughness Maps/cobblestone.png", cobblestoneRoughnessSRV);
	LoadTexture(L"../../Assets/Textures/Roughness Maps/scratched.png", scratchedRoughnessSRV);

	LoadTexture(L"../../Assets/Textures/Metal Maps/bronze.png", bronzeMetalSRV);
	LoadTexture(L"../../Assets/Textures/Metal Maps/cobblestone.png", cobblestoneMetalSRV);
	LoadTexture(L"../../Assets/Textures/Metal Maps/scratched.png", scratchedMetalSRV);


	srvAlbedoMapVector.push_back(bronzeAlbedoSRV);
	srvAlbedoMapVector.push_back(cobblestoneAlbedoSRV);
	srvAlbedoMapVector.push_back(scratchedAlbedoSRV);

	srvNormalMapVector.push_back(bronzeNormalSRV);
	srvNormalMapVector.push_back(cobblestoneNormalSRV);
	srvNormalMapVector.push_back(scratchedNormalSRV);

	srvRoughnessMapVector.push_back(bronzeRoughnessSRV);
	srvRoughnessMapVector.push_back(cobblestoneRoughnessSRV);
	srvRoughnessMapVector.push_back(scratchedRoughnessSRV);

	srvMetalMapVector.push_back(bronzeMetalSRV);
	srvMetalMapVector.push_back(cobblestoneMetalSRV);
	srvMetalMapVector.push_back(scratchedMetalSRV);

	//Create Materials
	CreateMaterials();


	for (int i = 0; i < materials.size(); i++)
	{
		std::shared_ptr<Material> mat = materials[i];

		mat->AddSampler("BasicSampler", samplerState);


		mat->AddTextureSRV("AlbedoMap", srvAlbedoMapVector[i % srvAlbedoMapVector.size()]);
		mat->AddTextureSRV("MetalnessMap", srvMetalMapVector[i % srvMetalMapVector.size()]);
		mat->AddTextureSRV("RoughnessMap", srvRoughnessMapVector[i % srvRoughnessMapVector.size()]);


		//top row uses flat normals
		if (i < 3)
			mat->AddTextureSRV("NormalMap", flatNormalSRV);

		//every other row uses their normals
		else
			mat->AddTextureSRV("NormalMap", srvNormalMapVector[i % srvNormalMapVector.size()]);
	}

	CreateEntites();

	CreateLights();
}

void Game::CreateLights()
{
	lights.push_back({});


	lights[0].Color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[0].Direction = DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f); //light points down
	lights[0].Intensity = 1.0f;
}


// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShaders.push_back(std::make_shared<SimpleVertexShader>(device, context,
		FixPath(L"VertexShader.cso").c_str()));
	pixelShaders.push_back(std::make_shared<SimplePixelShader>(device, context,
		FixPath(L"PixelShader.cso").c_str()));
	pixelShaders.push_back(std::make_shared<SimplePixelShader>(device, context,
		FixPath(L"CustomPixelShader.cso").c_str()));
}

void Game::CreateMaterials()
{
	for (int i = 0; i < 6; i++)
		materials.push_back(std::make_shared<Material>(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShaders[0], vertexShaders[0]));
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateEntites()
{

	size_t columnNum = (int)meshes.size();
	size_t flatMaterialNum = materials.size() / 2; //the number of flat materials (same as tthe number of non flat materials)
	for (int i = 0; i < entityNum; i++)
	{
		//if in top row, use the flat normal

		shared_ptr<Material> material;
		if (i / columnNum == 0)
			material = materials[i % (flatMaterialNum)];

		else
			material = materials[flatMaterialNum + (i % columnNum)];

		entities.push_back(Entity(meshes[i % meshes.size()], material));

		//move back so not in the same space as camera
		entities[i].GetTransform()->MoveAbsolute(0.0f, 0.0f, 3.0f);

		//move horizontally based on where you are in the list
		entities[i].GetTransform()->MoveAbsolute(-3.0f + ((i % columnNum) * 3.0f), 0.0f, 0.0f);

		//Move down based on the row you're on
		entities[i].GetTransform()->MoveAbsolute(0.0f, -3.0f * (i / columnNum), 0.0f);
	}
}

void Game::CameraInput(float deltaTime)
{
	Input& input = Input::GetInstance();
	DirectX::XMFLOAT3 moveVector = XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 rotateVectors = XMFLOAT3(0.0f, 0.0f, 0.0f);

	//W, S – Forward or backwards(This is relative movement)
	if (input.KeyDown('W'))
		moveVector.z = deltaTime;

	else if (input.KeyDown('S'))
		moveVector.z = -deltaTime;

	//A, D – Strafe left or right(Also relative movement)
	if (input.KeyDown('A'))
		moveVector.x = -deltaTime;

	else if (input.KeyDown('D'))
		moveVector.x = deltaTime;

	/* Q - Move up along the world’s Y axis(Absolute movement)
	   E - Move down along the world’s Y axis(Absolute movement)*/
	if (input.KeyDown('Q'))
		moveVector.y = deltaTime;

	else if (input.KeyDown('E'))
		moveVector.y = -deltaTime;

	if (input.MouseLeftDown())
		rotateVectors = XMFLOAT3((float)input.GetMouseYDelta(), (float)input.GetMouseXDelta(), 0.0f);

	cameras[activeCameraIndex]->Update(moveVector, rotateVectors);
}

// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	for (std::shared_ptr<Camera> camera : cameras)
		camera->UpdateProjectionMatrix((float)this->windowWidth / this->windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	ImGuiInitialization(deltaTime, this->windowHeight, this->windowWidth);

	if (rotate)
	{
		for (auto& e : entities)
			e.GetTransform()->Rotate(0, -deltaTime * 0.25f, 0);
	}

	CameraInput(deltaTime);


	

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();
}

void Game::ImGuiInitialization(float deltaTime, unsigned int windowHeight, unsigned int windowWidth)
{
	// Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)windowWidth;
	io.DisplaySize.y = (float)windowHeight;
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);

	if (ImGui::TreeNode("Controls"))
	{
		ImGui::Text("Q/E: Up/Down");
		ImGui::Text("W/S: Fowards/Backwards");
		ImGui::Text("A/D: Left/Right");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Active Camera Selection"))
	{
		ImGui::RadioButton("Camera 1", &activeCameraIndex, 0); ImGui::SameLine();
		ImGui::RadioButton("Camera 2", &activeCameraIndex, 1); ImGui::SameLine();
		ImGui::RadioButton("Camera 3", &activeCameraIndex, 2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Camera Information"))
	{
		for (int i = 0; i < cameras.size(); i++)
		{
			if (ImGui::TreeNode((void*)(intptr_t)i, "Camera %d", i + 1))
			{
				DirectX::XMFLOAT3 pos = cameras[i]->GetTransform()->GetPosition();
				ImGui::Text("Position: %f %f %f", pos.x, pos.y, pos.z);
				ImGui::Text("FOV (radiens): %f", cameras[i]->GetFieldOfView());
				ImGui::Text("Using Perspective View: %d", cameras[i]->UsingPerspectiveProjection());
				ImGui::TreePop();
			}
		}
	
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Entities"))
	{
		for (int i = 1; i < entityNum + 1; i++)
		{
			int index = i - 1;
			Entity e = entities[index];
			std::shared_ptr<Transform> t = e.GetTransform();

			if (ImGui::TreeNode((void*)(intptr_t)i, "Entity %d", i))
			{
				XMFLOAT3 pos = t->GetPosition();
				XMFLOAT3 rot = t->GetPitchYawRoll();
				XMFLOAT3 scale = t->GetScale();
				XMFLOAT4 colorTint = e.GetColorTint();

				if (ImGui::DragFloat3("Position", &pos.x, 0.01f, -10.0f, 10.0f))
					t->SetPosition(pos);

				if (ImGui::DragFloat3("Rotation (radians)", &rot.x, 0.01f, 0.0f, 6.28f))
					t->SetRotation(rot);

				if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.0f, 2.0f))
					t->SetScale(scale);
				
				if (ImGui::ColorEdit4("Color Tint", &colorTint.x))
					entities[index].SetColorTint(colorTint);

				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Lights"))
	{
		for (int i = 1; i < lights.size() + 1; i++)
		{
			if (ImGui::TreeNode((void*)(intptr_t)i, "Light %d", i))
			{
				int index = i - 1;
				
				//directional Light
				if (lights[index].Type == 0)
				{
					DirectX::XMFLOAT3 direction = lights[index].Direction;
					float intensity = lights[index].Intensity;


					if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f))
					{
						lights[index].Direction = direction;
					}

					if (ImGui::DragFloat("Inensity", &intensity, 0.01f, 0.0f, 10.0f))
					{
						lights[index].Intensity = intensity;
					}
				}

				ImGui::TreePop();

			}
		}
		
		ImGui::TreePop();

	}
	
	// Show the demo window
	//ImGui::ShowDemoWindow();
}



// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	//start drawing

	size_t columnNum = meshes.size();

	for (int i = 0; i < entityNum; i++)
	{
		Entity entity = entities[i];
		entity.GetMaterial()->SetLights("lights", &lights[0], sizeof(Light) * (int)lights.size());
		entity.GetMaterial()->SetTextureData();
		entity.GetMaterial()->GetPixelShader()->SetFloat3("ambient", DirectX::XMFLOAT3(0.59f, 0.42f, 0.52f));
		entity.GetMaterial()->GetPixelShader()->SetInt("lightNum", (int)lights.size());

		//only the third row should have the gamma correct
		int gammaNum = i / columnNum == 2;

		entity.GetMaterial()->GetPixelShader()->SetInt("useGammaCorrection", gammaNum);
		entity.GetMaterial()->GetPixelShader()->CopyAllBufferData();
		entity.Draw(cameras[activeCameraIndex]);
	}

	//draw skybox last
	skyBox->Draw(cameras[activeCameraIndex]);

	// Draw ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}
