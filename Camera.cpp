#include "Camera.h"

Camera::Camera(float aspectRatio)
{
	transform = std::make_shared<Transform>();
	ResetPosition();
	fieldOfViewAngle = DirectX::XM_PIDIV2;
	nearClipPlaneDistance = 0.01f;
	farPlaneDistance = 1000.0f;
	movementSpeed = 5.0f;
	mouseLookSpeed = 0.02f;
	orthographicWidth = 2.0f;
	perspectiveProjection = true;
	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

Camera::Camera(
	DirectX::XMFLOAT3 position,
	float moveSpeed,
	float mouseLookSpeed,
	float fieldOfViewAngle,
	float nearClipPlaneDistance,
	float farClipPlaneDistance,
	bool perspectiveProjection,
	float aspectRatio
)
{
	transform = std::make_shared<Transform>();
	transform->SetPosition(position);
	movementSpeed = moveSpeed;
	this->mouseLookSpeed = mouseLookSpeed;
	this->fieldOfViewAngle = fieldOfViewAngle;
	this->nearClipPlaneDistance = nearClipPlaneDistance;
	this->farPlaneDistance = farClipPlaneDistance;
	this->perspectiveProjection = perspectiveProjection;
	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
} 

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return projectionMatrix;
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
	DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
	if (perspectiveProjection)
	{
		matrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfViewAngle, aspectRatio,
								nearClipPlaneDistance, farPlaneDistance);
	}
	
	else
	{
		matrix = DirectX::XMMatrixOrthographicLH(orthographicWidth, 
												 orthographicWidth / aspectRatio, 
												 nearClipPlaneDistance, 
												 farPlaneDistance);
	}

	XMStoreFloat4x4(&projectionMatrix, matrix);
}
void Camera::ResetPosition()
{
	transform->SetPosition(0.0f, 0.0f, -1.0f);
	transform->SetRotation(0.0f, 0.0f, 0.0f);
	transform->SetScale(1.0f, 1.0f, 1.0f);
}
void Camera::SetFieldOfView(float fov, float aspectRatio)
{
	fieldOfViewAngle = fov;
	UpdateProjectionMatrix(aspectRatio);
}
void Camera::UpdateViewMatrix()
{
	DirectX::XMFLOAT3 position = transform->GetPosition();
	DirectX::XMFLOAT3 forward = transform->GetForward();
	DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(DirectX::XMLoadFloat3(&position),
													   DirectX::XMLoadFloat3(&forward), 
													   DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	XMStoreFloat4x4(&viewMatrix, view);
}
void Camera::Update(DirectX::XMFLOAT3 moveVectors, DirectX::XMFLOAT3 rotateVectors)
{
	moveVectors = DirectX::XMFLOAT3(moveVectors.x * movementSpeed, moveVectors.y * movementSpeed, moveVectors.z * movementSpeed);
	rotateVectors = DirectX::XMFLOAT3(rotateVectors.x * mouseLookSpeed, rotateVectors.y * mouseLookSpeed, 0.0f);

	transform->MoveRelative(moveVectors);
	transform->Rotate(rotateVectors);

	DirectX::XMFLOAT3 rotation = transform->GetPitchYawRoll();

	if (rotation.x > DirectX::XM_PIDIV2)
		rotation.x = DirectX::XM_PIDIV2;

	else if (rotation.x < -DirectX::XM_PIDIV2)
		rotation.x = -DirectX::XM_PIDIV2;

	transform->SetRotation(rotation);

	UpdateViewMatrix();
}

std::shared_ptr<Transform> Camera::GetTransform()
{
	return transform;
}

float Camera::GetFieldOfView()
{
	return fieldOfViewAngle;
}

bool Camera::UsingPerspectiveProjection()
{
	return perspectiveProjection;
}
