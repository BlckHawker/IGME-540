#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include <DirectXMath.h>
#include "Camera.h"
#include "Material.h"

class Entity
{
private:
	std::shared_ptr<Transform> object;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	DirectX::XMFLOAT4 colorTint;
	bool moveForward;

public:
	Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
	std::shared_ptr<Mesh> GetMesh();
	bool GetMoveForward();
	std::shared_ptr<Transform> GetTransform();
	DirectX::XMFLOAT4 GetColorTint();
	std::shared_ptr<Material> GetMaterial();
	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetMaterial(std::shared_ptr<Material> material);
	void SetMoveForward(bool moveForward);
	void Draw(std::shared_ptr<Camera> camera);
};

