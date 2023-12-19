#pragma once
#include "pch.h"

namespace seethe
{
struct InstanceData
{
	DirectX::XMFLOAT4X4 World;
	std::uint32_t MaterialIndex;
	std::uint32_t Pad0;
	std::uint32_t Pad1;
	std::uint32_t Pad2;
};
}