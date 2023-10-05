#pragma once
#include "pch.h"

namespace seethe
{
class InputLayout
{
public:
	InputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputs) noexcept : m_inputLayout(inputs) {}
	InputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>&& inputs) noexcept : m_inputLayout(std::move(inputs)) {}
	InputLayout(const InputLayout& rhs) noexcept : m_inputLayout(rhs.m_inputLayout) {}
	InputLayout(InputLayout&& rhs) noexcept : m_inputLayout(std::move(rhs.m_inputLayout)) {}
	InputLayout& operator=(const InputLayout& rhs) noexcept { m_inputLayout = rhs.m_inputLayout; return *this; }
	InputLayout& operator=(InputLayout&& rhs) noexcept { m_inputLayout = std::move(rhs.m_inputLayout); return *this; }
	~InputLayout() noexcept {}

	ND inline D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc() const noexcept { return { m_inputLayout.data(), (UINT)m_inputLayout.size() }; }


private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
};
}