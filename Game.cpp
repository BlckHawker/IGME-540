#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Mesh.h"
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

	Light l1 = {};
	Light l2 = {};
	Light l3 = {};
	Light l4 = {};
	Light l5 = {};

	l1.Color = DirectX::XMFLOAT3(1, 0, 0);
	l1.Direction = DirectX::XMFLOAT3(-1, 0, 0); //light comes from the right
	l1.Intensity = 1;

	l2.Color = DirectX::XMFLOAT3(0, 1, 0);
	l2.Direction = DirectX::XMFLOAT3(0, -1, 0); //light coems from up
	l2.Intensity = 1;

	l3.Color = DirectX::XMFLOAT3(0, 0, 1);
	l3.Direction = DirectX::XMFLOAT3(1, 0, 0); //light comes from the left
	l3.Intensity = 1;

	l4.Type = 1;
	l4.Color = DirectX::XMFLOAT3(1, 0, 1);
	l4.Range = 1;
	l4.Position = DirectX::XMFLOAT3(0, 0, 5); //light comes from the back
	l4.Intensity = .5;

	l5.Type = 1;
	l5.Color = DirectX::XMFLOAT3(1, 1, 0);
	l5.Range = 1;
	l5.Position = DirectX::XMFLOAT3(0, 0, -1); //light comes from the front
	l5.Intensity = .5;

	lights.push_back(l1);
	lights.push_back(l2);
	lights.push_back(l3);
	lights.push_back(l4);
	lights.push_back(l5);


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
	

	// Tell the input assembler (IA) stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our vertices?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

	XMFLOAT3 normal = XMFLOAT3(0, 0, -1);
	XMFLOAT2 uv = XMFLOAT2(0, 0);


	meshes.push_back(std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/cube.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/cylinder.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(device, context, FixPath("../../Assets/Models/sphere.obj").c_str()));


	

	materials.push_back(std::make_shared<Material>(1.0, DirectX::XMFLOAT4(1, 0, 0, 1), pixelShaders[0], vertexShaders[0]));
	materials.push_back(std::make_shared<Material>(1.0, DirectX::XMFLOAT4(0, 1, 0, 1), pixelShaders[0], vertexShaders[0]));
	materials.push_back(std::make_shared<Material>(1.0, DirectX::XMFLOAT4(0, 0, 1, 1), pixelShaders[0], vertexShaders[0]));
	materials.push_back(std::make_shared<Material>(1.0, DirectX::XMFLOAT4(1, 1, 1, 1), pixelShaders[0], vertexShaders[0]));



	for (int i = 0; i < entityNum; i++)
	{
		entities.push_back(Entity(meshes[i % meshes.size()], materials[3]));

		entities[i].GetTransform()->MoveAbsolute(0, 0, 3);
	}

	entities[0].GetTransform()->MoveAbsolute(-3, 0, 0);
	entities[2].GetTransform()->MoveAbsolute(3, 0, 0);

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
				float roughness = e.GetMaterial()->GetRoughness();


				if (ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f))
				{
					e.GetMaterial()->SetRoughness(roughness);
				}

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
		if (ImGui::TreeNode("Directional Light 1"))
		{
			DirectX::XMFLOAT3 direction = lights[0].Direction;
			float intensity = lights[0].Intensity;


			if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f))
			{
				lights[0].Direction = direction;
			}

			if (ImGui::DragFloat("Inensity", &intensity, 0.01f, 0.0f, 1.0f))
			{
				lights[0].Intensity = intensity;
			}

			ImGui::TreePop();

		}

		if (ImGui::TreeNode("Directional Light 2"))
		{
			DirectX::XMFLOAT3 direction = lights[1].Direction;
			float intensity = lights[1].Intensity;


			if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f))
			{
				lights[1].Direction = direction;
			}

			if (ImGui::DragFloat("Inensity", &intensity, 0.01f, 0.0f, 1.0f))
			{
				lights[1].Intensity = intensity;
			}

			ImGui::TreePop();

		}

		if (ImGui::TreeNode("Directional Light 3"))
		{
			DirectX::XMFLOAT3 direction = lights[2].Direction;
			float intensity = lights[2].Intensity;


			if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f))
			{
				lights[2].Direction = direction;
			}

			if (ImGui::DragFloat("Inensity", &intensity, 0.01f, 0.0f, 1.0f))
			{
				lights[2].Intensity = intensity;
			}

			ImGui::TreePop();

		}

		if (ImGui::TreeNode("Point Light 1"))
		{
			DirectX::XMFLOAT3 position = lights[3].Position;
			float range = lights[3].Range;
			float intensity = lights[3].Intensity;


			if (ImGui::DragFloat3("Position", &position.x, 0.01f, -10.0f, 10.0f))
			{
				lights[3].Position = position;
			}

			if (ImGui::DragFloat("Inensity", &intensity, 0.01f, 0.0f, 1.0f))
			{
				lights[3].Intensity = intensity;
			}

			if (ImGui::DragFloat("Range", &range, 0.01f, 0.0f, 1.0f))
			{
				lights[3].Range = range;
			}

			ImGui::TreePop();

		}

		if (ImGui::TreeNode("Point Light 2"))
		{
			DirectX::XMFLOAT3 position = lights[4].Position;
			float range = lights[4].Range;
			float intensity = lights[4].Intensity;


			if (ImGui::DragFloat3("Position", &position.x, 0.01f, -10.0f, 10.0f))
			{
				lights[4].Position = position;
			}

			if (ImGui::DragFloat("Inensity", &intensity, 0.01f, 0.0f, 1.0f))
			{
				lights[4].Intensity = intensity;
			}

			if (ImGui::DragFloat("Range", &range, 0.01f, 0.0f, 1.0f))
			{
				lights[4].Range = range;
			}

			ImGui::TreePop();

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
	for (Entity entity : entities)
	{
		entity.GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		entity.Draw(cameras[activeCameraIndex]);
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
