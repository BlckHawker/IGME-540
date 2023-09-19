#include "Transform.h"
Transform::Transform()
{
	position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	dirtyMatrix = false;
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, DirectX::XMMatrixIdentity());
}

void Transform::UpdateMatrices()
{
	//To actually create a world matrix, create separate translation, rotation and scale matrices, multiply them together and store the result
	// track whether or not any of the 3 major transformations have changed, meaning the matrix is “dirty” and needs an update.You would 
	//then update the matrix right before it's actually returned by a GetWorldMatrix() method, and only if it's dirty.

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX sc = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX rot = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	DirectX::XMMATRIX world = XMMatrixMultiply(XMMatrixMultiply(sc, rot), translation);

	XMStoreFloat4x4(&worldMatrix, world);
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixInverse(0, XMMatrixTranspose(world)));
	dirtyMatrix = false;
}

void Transform::SetPosition(float x, float y, float z)
{
	if (position.x == x && position.y == y && position.z == z)
	{
		return;
	}
	position = DirectX::XMFLOAT3(x, y, z);
	dirtyMatrix = true;
}

void Transform::SetPosition(DirectX::XMFLOAT3 position)
{
	SetPosition(position.x, position.y, position.z);
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	if (rotation.x == pitch && rotation.y == yaw && rotation.z == roll)
		return;
	rotation = DirectX::XMFLOAT3(pitch, yaw, roll);
	dirtyMatrix = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	SetRotation(rotation.x, rotation.y, rotation.z);
}

void Transform::SetScale(float x, float y, float z)
{
	if (scale.x == x && scale.y == y && scale.z == z)
		return;
	scale = DirectX::XMFLOAT3(x, y, z);
	dirtyMatrix = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	SetScale(scale.x, scale.y, scale.z);
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return rotation;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	if(dirtyMatrix)
		UpdateMatrices();
	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	if(dirtyMatrix)
		UpdateMatrices();
	return worldInverseTransposeMatrix;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	if (x == 0 && y == 0 && z == 0)
		return;
	position = DirectX::XMFLOAT3(position.x + x, position.y + y, position.z + z);
	dirtyMatrix = true;
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	MoveAbsolute(offset.x, offset.y, offset.z);
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	if (pitch == 0 && yaw == 0 && roll == 0)
		return;
	rotation = DirectX::XMFLOAT3(rotation.x + pitch, rotation.y + yaw, rotation.z + roll);
	dirtyMatrix = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	Rotate(rotation.x, rotation.y, rotation.z);
}

void Transform::Scale(float x, float y, float z)
{
	if (x == 1 && y == 1 && z == 1)
		return;

	scale = DirectX::XMFLOAT3(scale.x * x, scale.y * y, scale.z * z);
	dirtyMatrix = true;
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	Scale(scale.x, scale.y, scale.z);
}
