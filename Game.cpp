#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Mesh.h"
#include "BufferStructs.h"
#include <memory>
#include <vector>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

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
				xPos = -1;
				yRotation = DirectX::XM_PIDIV4;
				fov = DirectX::XM_PIDIV4;
				break;
			case 2:
				xPos = 1;
				yRotation = -DirectX::XM_PIDIV4;
				fov = DirectX::XM_PI / 3;
				break;
		}

		if (i != 1)
		{
			cameras[i]->GetTransform()->MoveAbsolute(xPos, 0, 0);
			cameras[i]->GetTransform()->Rotate(0.0f, yRotation, 0.0f);
			cameras[i]->SetFieldOfView(fov, aspectRatio);
		}

	}
	
	unsigned int size = (sizeof(VertexShaderExternalData) + 15) / 16 * 16;

	 // Describe the constant buffer
	 D3D11_BUFFER_DESC cbDesc = {}; // Sets struct to all zeros
	 cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	 cbDesc.ByteWidth = size; // Must be a multiple of 16
	 cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	 cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	 //create the buffer
	 device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf());

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
	CreateGeometry();
	
	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		context->VSSetShader(vertexShader.Get(), 0, 0);
		context->PSSetShader(pixelShader.Get(), 0, 0);
	}
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
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 yellow = XMFLOAT4(1.0f, 0.84f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 orange = XMFLOAT4(1.0f, 0.54f, 0.0f, 1.0f);

	Vertex triangleVertices[] =
	{
		{ XMFLOAT3(+0.0f, +0.2f, +0.0f), red },
		{ XMFLOAT3(+0.2f, -0.2f, +0.0f), blue },
		{ XMFLOAT3(-0.2f, -0.2f, +0.0f), green },
	};

	unsigned int triangleIndices[] = { 0, 1, 2 };

	Vertex squareVertices[] =
	{
		{ XMFLOAT3(-0.9f, -0.0f, +0.0f), red },
		{ XMFLOAT3(-0.4f, -0.0f, +0.0f), orange},
		{ XMFLOAT3(-0.4f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.9f, -0.5f, +0.0f), black },
	};

	unsigned int squareIndices[] = { 0, 1, 2, 0, 2, 3 };

	Vertex arrowVerticies[] =
	{
		{ XMFLOAT3(+0.25f, +0.25f, +0.0f), black },
		{ XMFLOAT3(+0.25f, +0.75f, +0.0f), black},
		{ XMFLOAT3(+0.75f, +0.25f, +0.0f), black },
		{ XMFLOAT3(+0.25f, +0.5f, +0.0f), black },
		{ XMFLOAT3(+0.5f, +0.5f, +0.0f), black },
		{ XMFLOAT3(+0.25f, -0.25f, +0.0f), black },
	};

	unsigned int arrowIndices[] = { 0, 1, 2, 3, 4, 5 };

	meshes.push_back(std::make_shared<Mesh>(triangleVertices, 3, triangleIndices, 3, device, context));
	meshes.push_back(std::make_shared<Mesh>(squareVertices,   4, squareIndices,   6, device, context));
	meshes.push_back(std::make_shared<Mesh>(arrowVerticies,   6, arrowIndices,    6, device, context));

	for (int i = 0; i < entityNum; i++)
	{
		entities.push_back(Entity(meshes[i % meshes.size()]));
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
	
	if (automaticTranslation)
		entities[0].GetTransform()->SetPosition(sinf(totalTime),0,0);

	if (automaticRotation)
	{
		rotationValue += 2.0f * deltaTime;
		if (rotationValue > 6.28f)
			rotationValue = 0;

		XMFLOAT3 rot = entities[1].GetTransform()->GetPitchYawRoll();
		rot.z = rotationValue;
		entities[1].GetTransform()->SetRotation(rot);
	}

	if (automaticScaling)
		entities[2].GetTransform()->SetScale(1, sinf(totalTime), 1);

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

	if (ImGui::TreeNode("Automatic Transformations"))
	{
		ImGui::Checkbox("Automatic Translation", &automaticTranslation);
		ImGui::Checkbox("Automatic Scaling", &automaticScaling);
		if (ImGui::Checkbox("Automatic Rotation", &automaticRotation) && automaticRotation)
			rotationValue = entities[1].GetTransform()->GetPitchYawRoll().z;
		ImGui::TreePop();
	}

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


				if (ImGui::DragFloat3("Position", &pos.x, 0.01f, -1.0f, 1.0f))
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
	
	// Show the demo window
	ImGui::ShowDemoWindow();
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

	context->VSSetConstantBuffers(
		0, // Which slot (register) to bind the buffer to?
		1, // How many are we activating? Can do multiple at once
		vsConstantBuffer.GetAddressOf()); // Array of buffers (or the address of one)

	//start drawing
	for (Entity entity : entities)
	{
		entity.Draw(vsConstantBuffer, cameras[activeCameraIndex]);
	}

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
