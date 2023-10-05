#pragma once
#include <DirectXMath.h>
class Transform
{
private:
	DirectX::XMFLOAT3 position, scale, rotation;
	DirectX::XMFLOAT3 up, right, forward;

	DirectX::XMFLOAT4X4 worldMatrix, worldInverseTransposeMatrix;

	bool dirtyMatrix;
	bool dirtyDirections;
	void UpdateMatrices();
	void UpateDirections(); //helper method to get the transform's realtive up, right, and forward

public:
	Transform();
	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 rotation);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 scale);
	DirectX::XMFLOAT3 GetRight(); // Rotate the world’s right vector(1, 0, 0) by the transform’s pitch / yaw / roll.
	DirectX::XMFLOAT3 GetUp(); // Rotate the world’s up vector(0, 1, 0) by the transform’s pitch / yaw / roll.
	DirectX::XMFLOAT3 GetForward(); // Rotate the world’s forward vector(0, 0, 1) by the transform’s pitch / yaw / roll.
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 offset);
	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 rotation);
	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 scale);

};