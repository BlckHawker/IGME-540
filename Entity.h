#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include "BufferStructs.h"
#include <DirectXMath.h>
#include "Camera.h"

class Entity
{
private:
	std::shared_ptr<Transform> object;
	std::shared_ptr<Mesh> mesh;
	DirectX::XMFLOAT4 colorTint;
public:
	Entity(std::shared_ptr<Mesh> mesh);
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	DirectX::XMFLOAT4 GetColorTint();
	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void Draw(Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer, std::shared_ptr<Camera> camera);


};

