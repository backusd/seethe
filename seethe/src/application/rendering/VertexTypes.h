#pragma once
#include "pch.h"



namespace seethe
{
struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;

	// This method is required because we impose the HAS_POSITION concept on MeshGroupT so that we can compute the bounding box
	ND inline DirectX::XMFLOAT3 Position() const noexcept { return Pos; }
};
struct SolidColorVertex
{
	DirectX::XMFLOAT4 Pos;

	// This method is required because we impose the HAS_POSITION concept on MeshGroupT so that we can compute the bounding box
	ND inline DirectX::XMFLOAT3 Position() const noexcept { return { Pos.x, Pos.y, Pos.z }; }
};
}