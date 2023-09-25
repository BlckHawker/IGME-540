#include "Entity.h"
#include "Transform.h"
// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

Entity::Entity(std::shared_ptr<Mesh> mesh)
{
	object = std::make_shared<Transform>();
	this->mesh = mesh;
	colorTint = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}
std::shared_ptr<Mesh> Entity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> Entity::GetTransform()
{
	return object;
}

void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer, std::shared_ptr<Camera> camera)
{
	//Set the correct Constant Buffer resource for the Vertex Shader stage (done in assignment 3): 
	// -Done in game.cpp
	
	//Collect data for the current entity in a C++ struct (done in assignment 3, but now updated to
	//hold the world matrix from the entity you're about to draw): 
	VertexShaderExternalData vsData;

	vsData.colorTint = colorTint;
	vsData.worldMatrix = object->GetWorldMatrix();
	vsData.projectionMatrix = camera->GetProjectionMatrix();
	vsData.viewMatrix = camera->GetViewMatrix();

	//Map / memcpy / Unmap the Constant Buffer resource(done in assignment 3)
	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	mesh->GetContext()->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
	mesh->GetContext()->Unmap(vsConstantBuffer.Get(), 0);

	//Set the correct Vertex and Index Buffers(done in assignment 2)
	// -Done in mesh.cpp
	//Tell D3D to render using the currently bound resources(done in assignment 2)
	// -Done in game.cpp
	mesh->Draw();
}

DirectX::XMFLOAT4 Entity::GetColorTint()
{
	return colorTint;
}

void Entity::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
	this->colorTint = colorTint;
}
