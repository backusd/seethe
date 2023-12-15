#pragma once
#include "pch.h"

namespace seethe
{
class InputLayout
{
public:
	constexpr InputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputs) noexcept : m_inputLayout(inputs) {}
	constexpr InputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>&& inputs) noexcept : m_inputLayout(std::move(inputs)) {}
	constexpr InputLayout(const InputLayout& rhs) noexcept : m_inputLayout(rhs.m_inputLayout) {}
	constexpr InputLayout(InputLayout&& rhs) noexcept : m_inputLayout(std::move(rhs.m_inputLayout)) {}
	constexpr InputLayout& operator=(const InputLayout& rhs) noexcept { m_inputLayout = rhs.m_inputLayout; return *this; }
	constexpr InputLayout& operator=(InputLayout&& rhs) noexcept { m_inputLayout = std::move(rhs.m_inputLayout); return *this; }
	~InputLayout() noexcept {}

	ND constexpr D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc() const noexcept { return { m_inputLayout.data(), (UINT)m_inputLayout.size() }; }


private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
};
}