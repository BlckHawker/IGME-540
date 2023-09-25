#pragma once
#include "Input.h"
#include <memory>
#include "Transform.h"
#include <DirectXMath.h>
#include <iostream>
class Camera
{
private:
	std::shared_ptr<Transform> transform;
	DirectX::XMFLOAT4X4 viewMatrix, projectionMatrix;
	float fieldOfViewAngle; //in radiens
	float nearClipPlaneDistance, farPlaneDistance;
	float movementSpeed, mouseLookSpeed, orthographicWidth;
	bool perspectiveProjection; //orthographic if false
	void UpdateViewMatrix();

public:
	Camera(float aspectRatio);
	Camera(
		DirectX::XMFLOAT3 position,
		float moveSpeed,
		float mouseLookSpeed,
		float fieldOfViewAngle,
		float nearClipPlaneDistance,
		float farClipPlaneDistance,
		bool projectionPerspective,
		float aspectRatio
	);
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	void UpdateProjectionMatrix(float aspectRatio);
	void Update(DirectX::XMFLOAT3 moveVectors, DirectX::XMFLOAT3 rotateVectors);
	void ResetPosition();
	void SetFieldOfView(float fov, float aspectRation);
	float GetFieldOfView();
	bool UsingPerspectiveProjection();
	std::shared_ptr<Transform> GetTransform();
};

