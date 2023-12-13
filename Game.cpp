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

	shadowViewMatrix = XMFLOAT4X4();
	shadowProjectionMatrix = XMFLOAT4X4();

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

void Game::CreatePostProcessingResources()
{
	ppRTV.Reset();
	ppSRV.Reset();

	// Describe the texture we're creating
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = windowWidth;
	textureDesc.Height = windowHeight;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture;
	device->CreateTexture2D(&textureDesc, 0, ppTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(
		ppTexture.Get(),
		&rtvDesc,
		ppRTV.ReleaseAndGetAddressOf());
	// Create the Shader Resource View
	// By passing it a null description for the SRV, we
	// get a "default" SRV that has access to the entire resource
	device->CreateShaderResourceView(
		ppTexture.Get(),
		0,
		ppSRV.ReleaseAndGetAddressOf());
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

	//Post processing set up
	D3D11_SAMPLER_DESC ppSampDesc = {};
	ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());

	//render target creation
	//set up shaders
	ppVS = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"FullScreenVertexShader.cso").c_str());
	ppPS = std::make_shared<SimplePixelShader>(device, context, FixPath(L"BoxBlurPostProcessPixelShader.cso").c_str());

	CreatePostProcessingResources();

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
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeAlbedoSRV, cobblestoneAlbedoSRV, scratchedAlbedoSRV, woodAlbedoSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormalSRV, cobblestoneNormalSRV, scratchedNormalSRV, flatNormalSRV, woodNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughnessSRV, cobblestoneRoughnessSRV, scratchedRoughnessSRV, woodRoughnessSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeMetalSRV, cobblestoneMetalSRV, scratchedMetalSRV, woodMetalSRV;

	//================================================================================

	LoadTexture(L"../../Assets/Textures/Albedo Maps/bronze.png", bronzeAlbedoSRV);
	LoadTexture(L"../../Assets/Textures/Albedo Maps/cobblestone.png", cobblestoneAlbedoSRV);
	LoadTexture(L"../../Assets/Textures/Albedo Maps/scratched.png", scratchedAlbedoSRV);
	LoadTexture(L"../../Assets/Textures/Albedo Maps/wood.png", woodAlbedoSRV);

	LoadTexture(L"../../Assets/Textures/Normal Maps/bronze.png", bronzeNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/cobblestone.png", cobblestoneNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/scratched.png", scratchedNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/flat.png", flatNormalSRV);
	LoadTexture(L"../../Assets/Textures/Normal Maps/wood.png", woodNormalSRV);

	LoadTexture(L"../../Assets/Textures/Roughness Maps/bronze.png", bronzeRoughnessSRV);
	LoadTexture(L"../../Assets/Textures/Roughness Maps/cobblestone.png", cobblestoneRoughnessSRV);
	LoadTexture(L"../../Assets/Textures/Roughness Maps/scratched.png", scratchedRoughnessSRV);
	LoadTexture(L"../../Assets/Textures/Roughness Maps/wood.png", woodRoughnessSRV);

	LoadTexture(L"../../Assets/Textures/Metal Maps/bronze.png", bronzeMetalSRV);
	LoadTexture(L"../../Assets/Textures/Metal Maps/cobblestone.png", cobblestoneMetalSRV);
	LoadTexture(L"../../Assets/Textures/Metal Maps/scratched.png", scratchedMetalSRV);
	LoadTexture(L"../../Assets/Textures/Metal Maps/wood.png", woodMetalSRV);

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

	floorMaterial->AddSampler("BasicSampler", samplerState);
	floorMaterial->AddTextureSRV("AlbedoMap", woodAlbedoSRV);
	floorMaterial->AddTextureSRV("NormalMap", woodNormalSRV);
	floorMaterial->AddTextureSRV("MetalnessMap", woodMetalSRV);
	floorMaterial->AddTextureSRV("RoughnessMap", woodRoughnessSRV);


	CreateEntites();

	CreateLights();

	CreateShadowMapResources();
}

void Game::CreateLights()
{
	lights.push_back({});


	lights[0].Color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[0].Direction = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f); //light points down
	lights[0].Intensity = 2.0f;
}

void Game::CreateShadowMapResources()
{
	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution; 
	shadowDesc.Height = shadowMapResolution;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS; //reserve all 32 bits for a single value
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture; //not needed after this method
	device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	//views to bind texture resources in the pipeline (shadowDSV and shadowSRV)
	//ViewDimension describes the dimensionality of the resource. In this case it’s just a 2D texture.
    //MipSlice tells the depth view which mip map to render into. We only have 1, so it’s index 0.
	//MipLevels and MostDetailedMip tell the SRV which mips it can read. Again, we only have 1.
	//Format is slightly different for each view (D32_FLOAT vs. R32_FLOAT). These both mean “treat all 32 bits as a single value”, but D32_FLOAT is specific to depth views.

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView( shadowTexture.Get(), &shadowDSDesc, shadowDSV.GetAddressOf());
	
	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture.Get(), &srvDesc, shadowSRV.GetAddressOf());

	//To render from the light’s point of view, we’ll need view and projection matrices that match the light

	//View matrix creation requires three things: a position, a direction and an up vector.
	//up vector is world's up
	//direction should match whichever directional light is casting shadows in your scene
	//Position is the trickiest as directional lights don’t inherently have a position. As discussed above, picking a 
	//point close to the center of your game world(perhaps the origin) and backing up along the negative light direction 
	//works well for a simple scene

	XMMATRIX lightView = XMMatrixLookToLH(
		XMVectorSet(0, 20, 0, 0), // Position: "Backing up" 20 units from origin
		XMVectorSet(lights[0].Direction.x, lights[0].Direction.y, lights[0].Direction.z, 0), // Direction: light's direction
		XMVectorSet(1, 0, 0, 0));


	float lightProjectionSize = 25.0f; // how much of the world is included within the shadow map
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);

	XMStoreFloat4x4(&shadowViewMatrix, lightView);
	XMStoreFloat4x4(&shadowProjectionMatrix, lightProjection);

	//fix shadow achne
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);



	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);
}

void Game::RenderShadowMap()
{
	context->RSSetState(shadowRasterizer.Get());

	//clear the shadow map
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	//Set up the output merger stage
	ID3D11RenderTargetView* nullRTV{};
	context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

	//Deactivate pixel shader
	context->PSSetShader(0, 0, 0);

	//Change viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	//Entity render loop
	std::shared_ptr<SimpleVertexShader> shadowVS = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"ShadowVertexShader.cso").c_str());
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", shadowViewMatrix);
	shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);
	// Loop and draw all entities
	for (auto& e : entities)
	{
		shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
		shadowVS->CopyAllBufferData();
		// Draw the mesh directly to avoid the entity's material
		// Note: Your code may differ significantly here!
		e->GetMesh()->Draw();
	}

	//Reset the pipeline
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);
	context->RSSetState(0);
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

	floorMaterial = std::make_shared<Material>(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShaders[0], vertexShaders[0]);
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

		entities.push_back(make_shared<Entity>(meshes[i % meshes.size()], material));

		//move back so not in the same space as camera
		entities[i]->GetTransform()->MoveAbsolute(0.0f, 0.0f, 3.0f);

		//move horizontally based on where you are in the list
		entities[i]->GetTransform()->MoveAbsolute(-3.0f + ((i % columnNum) * 3.0f), 0.0f, 0.0f);

		//Move down based on the row you're on
		entities[i]->GetTransform()->MoveAbsolute(0.0f, -3.0f * (i / columnNum), 0.0f);
	}

	//create floor entity
	floorEntity = std::make_shared<Entity>(meshes[0], floorMaterial);
	floorEntity->GetTransform()->MoveAbsolute(0.0, -8.0f, -3.0f);
	floorEntity->GetTransform()->Scale(10.0f, 0.1f, 10.0f);

	//change the first three entities z pos for assignment 11
	entities[0]->GetTransform()->MoveAbsolute(0.0f, 0.0f, -10.0f);
	entities[1]->GetTransform()->MoveAbsolute(0.0f, 0.0f, 3.0f);
	entities[2]->GetTransform()->MoveAbsolute(0.0f, 0.0f, 0.0f);
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

	CreatePostProcessingResources();
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	ImGuiInitialization(deltaTime, this->windowHeight, this->windowWidth);

	float maxZ = 3.0f;
	float minZ = -10.0f;
	float moveAmount = 5.0f;

	if (rotate)
	{
		for (int i = 0; i < entities.size(); i++)
		{
			if (i == entities.size() - 1)
				break;

			entities[i]->GetTransform()->Rotate(0, -deltaTime * 0.25f, 0);
		}

		for (int i = 0; i < 3; i++)
		{
			//make it so only the first row moves back and forth
			if (entities[i]->GetMoveForward())
			{
				entities[i]->GetTransform()->MoveAbsolute(0, 0, moveAmount * deltaTime);
				if (entities[i]->GetTransform()->GetPosition().z >= maxZ)
					entities[i]->SetMoveForward(false);
			}

			else
			{
				entities[i]->GetTransform()->MoveAbsolute(0, 0, -moveAmount * deltaTime);
				if (entities[i]->GetTransform()->GetPosition().z <= minZ)
					entities[i]->SetMoveForward(true);
			}
		}
	}

	CameraInput(deltaTime);
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

	if (ImGui::DragInt("Blur", &blurAmount, 1, 0, 10))
	{
		blurAmount = blurAmount;
	}


	if (ImGui::TreeNode("Controls"))
	{
		ImGui::Text("Q/E: Up/Down");
		ImGui::Text("W/S: Fowards/Backwards");
		ImGui::Text("A/D: Left/Right");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Shadow Map Image Debugger"))
	{
		ImGui::Image(shadowSRV.Get(), ImVec2(512, 512));
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
			shared_ptr<Entity> e = entities[index];
			std::shared_ptr<Transform> t = e->GetTransform();

			if (ImGui::TreeNode((void*)(intptr_t)i, "Entity %d", i))
			{
				XMFLOAT3 pos = t->GetPosition();
				XMFLOAT3 rot = t->GetPitchYawRoll();
				XMFLOAT3 scale = t->GetScale();
				XMFLOAT4 colorTint = e->GetColorTint();

				if (ImGui::DragFloat3("Position", &pos.x, 0.01f, -10.0f, 10.0f))
					t->SetPosition(pos);

				if (ImGui::DragFloat3("Rotation (radians)", &rot.x, 0.01f, 0.0f, 6.28f))
					t->SetRotation(rot);

				if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.0f, 2.0f))
					t->SetScale(scale);
				
				if (ImGui::ColorEdit4("Color Tint", &colorTint.x))
					entities[index]->SetColorTint(colorTint);

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

	//post precessing - Pre Render
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
	//context->ClearRenderTargetView(ppRTV.Get(), clearColor);


	
	//start drawing - redering as normal
	size_t columnNum = meshes.size();

	//Shadow map
	RenderShadowMap();

	context->OMSetRenderTargets(1, ppRTV.GetAddressOf(), depthBufferDSV.Get());
	//swap acive rendering target

	for (int i = 0; i < entityNum; i++)
	{
		shared_ptr<Entity> entity = entities[i];
		entity->GetMaterial()->SetLights("lights", &lights[0], sizeof(Light) * (int)lights.size());

		entity->GetMaterial()->GetVertexShader()->SetMatrix4x4("shadowView", shadowViewMatrix);
		entity->GetMaterial()->GetVertexShader()->SetMatrix4x4("shadowProjection", shadowProjectionMatrix);
		
		entity->GetMaterial()->SetTextureData();
		entity->GetMaterial()->GetPixelShader()->SetFloat3("ambient", DirectX::XMFLOAT3(0.59f, 0.42f, 0.52f));
		entity->GetMaterial()->GetPixelShader()->SetInt("lightNum", (int)lights.size());

		//only the third row should have the gamma correct
		int gammaNum = i / columnNum == 2;

		entity->GetMaterial()->GetPixelShader()->SetInt("useGammaCorrection", gammaNum);
		entity->GetMaterial()->GetPixelShader()->SetShaderResourceView("ShadowMap", shadowSRV);
		entity->GetMaterial()->GetPixelShader()->SetSamplerState("ShadowSampler", shadowSampler);
		entity->GetMaterial()->GetPixelShader()->CopyAllBufferData();
		entity->Draw(cameras[activeCameraIndex]);
	}

	floorEntity->GetMaterial()->SetLights("lights", &lights[0], sizeof(Light) * (int)lights.size());
	floorEntity->GetMaterial()->GetVertexShader()->SetMatrix4x4("shadowView", shadowViewMatrix);
	floorEntity->GetMaterial()->GetVertexShader()->SetMatrix4x4("shadowProjection", shadowProjectionMatrix);
	floorEntity->GetMaterial()->SetTextureData();
	floorEntity->GetMaterial()->GetPixelShader()->SetFloat3("ambient", DirectX::XMFLOAT3(0.59f, 0.42f, 0.52f));
	floorEntity->GetMaterial()->GetPixelShader()->SetInt("lightNum", (int)lights.size());
	floorEntity->GetMaterial()->GetPixelShader()->SetInt("useGammaCorrection", 1);
	floorEntity->GetMaterial()->GetPixelShader()->SetShaderResourceView("ShadowMap", shadowSRV);
	floorEntity->GetMaterial()->GetPixelShader()->SetSamplerState("ShadowSampler", shadowSampler);
	floorEntity->GetMaterial()->GetPixelShader()->CopyAllBufferData();
	floorEntity->Draw(cameras[activeCameraIndex]);

	//draw skybox last
	skyBox->Draw(cameras[activeCameraIndex]);

	//post processing - post render
	BloomPostProcessing();

	//unbind the srv
	ID3D11ShaderResourceView* nullSRVs[128] = {};
	context->PSSetShaderResources(0, 128, nullSRVs);

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

void Game::BloomPostProcessing()
{
	// Activate shaders and bind resources
	// Also set any required cbuffer data
	//render to the back buffer
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);
	
	ppPS->SetInt("blurRadius", blurAmount);
	ppPS->SetFloat("pixelWidth", 1.0f / windowWidth);
	ppPS->SetFloat("pixelHeight", 1.0f / windowHeight);
	ppPS->CopyAllBufferData();
	ppVS->SetShader();
	ppPS->SetShader();
	ppPS->SetShaderResourceView("Pixels", ppSRV.Get());
	ppPS->SetSamplerState("ClampSampler", ppSampler.Get());
	// Draw exactly 3 vertices for our "full screen triangle"
	context->Draw(3, 0); // Draw exactly 3 vertices (one triangle)

}
