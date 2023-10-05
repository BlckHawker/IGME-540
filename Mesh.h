#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "Vertex.h"
#include <vector>

//hold geometry data (vertices & indices) in Direct3D buffers

class Mesh
{

public:
	Mesh(Vertex* vertexObjects,
		int vertexCount,
		unsigned int* indices,
		int indexCount,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

	Mesh(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, const char* fileName);

	~Mesh();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer(); //method to return the pointer to the vertex buffer object
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer(); //method, which does the same thing for the index buffer
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext();
	int GetIndexCount(); //method, which returns the number of indices this mesh contains
	void Draw(); //method, which sets the buffers and tells DirectX to draw the correct number of indices
private:
	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	int indexCount;
	void CreateVertexAndIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Device> device, Vertex* vertexObjects, int vertexCount, unsigned int* indices);
};

