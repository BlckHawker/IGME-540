#include "Entity.h"
#include "Transform.h"
// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

Entity::Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
{
	object = std::make_shared<Transform>();
	colorTint = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	this->mesh = mesh;
	this->material = material;
}
std::shared_ptr<Mesh> Entity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> Entity::GetTransform()
{
	return object;
}

void Entity::Draw(std::shared_ptr<Camera> camera)
{
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();

	vs->SetFloat4("colorTint", material->GetColorTint()); 
	vs->SetMatrix4x4("worldMatrix", object->GetWorldMatrix());
	vs->SetMatrix4x4("projectionMatrix", camera->GetProjectionMatrix());
	vs->SetMatrix4x4("viewMatrix", camera->GetViewMatrix());

	vs->CopyAllBufferData();

	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();


	mesh->Draw();
}

DirectX::XMFLOAT4 Entity::GetColorTint()
{
	return colorTint;
}

std::shared_ptr<Material> Entity::GetMaterial()
{
	return material;
}

void Entity::SetMaterial(std::shared_ptr<Material> material)
{
	this->material = material;
}

void Entity::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
	this->colorTint = colorTint;
}
