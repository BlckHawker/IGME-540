#include "Transform.h"
Transform::Transform()
{
	right = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	forward = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);

	position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	rotation = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);

	dirtyDirections = false;
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

void Transform::UpateDirections()
{
	DirectX::XMVECTOR quternionRotation = DirectX::XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMStoreFloat3(&right, DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), quternionRotation));
	XMStoreFloat3(&up, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), quternionRotation));
	XMStoreFloat3(&forward, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), quternionRotation));
	dirtyDirections = false;
}


void Transform::SetPosition(float x, float y, float z)
{
	if (position.x == x && position.y == y && position.z == z)
		return;
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
	dirtyDirections = true;

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

DirectX::XMFLOAT3 Transform::GetRight()
{
	if (dirtyDirections)
		UpateDirections();
	return right;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
	if (dirtyDirections)
		UpateDirections();
	return up;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
	if (dirtyDirections)
		UpateDirections();
	return forward;
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

void Transform::MoveRelative(float x, float y, float z)
{
	if (x == 0.0f && y == 0.0f && z == 0.0f)
		return;

	//this code is very similar to the demo because I was reffering to it while trying to fix a bug
	DirectX::XMVECTOR relativeMovement = DirectX::XMVectorSet(x,y,z,0);
	DirectX::XMVECTOR rot = DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&rotation));
	DirectX::XMVECTOR direction = DirectX::XMVector3Rotate(relativeMovement, rot);
	DirectX::XMVECTOR newPosition = DirectX::XMVectorAdd(XMLoadFloat3(&position), direction);
	XMStoreFloat3(&position, newPosition);
	dirtyMatrix = true;
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	MoveRelative(offset.x, offset.y, offset.z);
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	if (x == 0.0f && y == 0.0f && z == 0.0f)
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
	if (pitch == 0.0f && yaw == 0.0f && roll == 0.0f)
		return;
	rotation = DirectX::XMFLOAT3(rotation.x + pitch, rotation.y + yaw, rotation.z + roll);
	dirtyDirections = true;
	dirtyMatrix = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	Rotate(rotation.x, rotation.y, rotation.z);
}

void Transform::Scale(float x, float y, float z)
{
	if (x == 1.0f && y == 1.0f && z == 1.0f)
		return;

	scale = DirectX::XMFLOAT3(scale.x * x, scale.y * y, scale.z * z);
	dirtyMatrix = true;
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	Scale(scale.x, scale.y, scale.z);
}
