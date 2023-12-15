#pragma once
#include "pch.h"
#include "ConstantBuffer.h"
#include "utils/Timer.h"



namespace seethe
{
class RootConstantBufferView
{
public:
	inline RootConstantBufferView(UINT rootParameterIndex, ConstantBufferBase* cb) noexcept :
		m_rootParameterIndex(rootParameterIndex),
		m_constantBuffer(cb)
	{
		ASSERT(m_constantBuffer != nullptr, "ConstantBuffer should not be nullptr");
	}
	RootConstantBufferView(const RootConstantBufferView&) noexcept = default;
	RootConstantBufferView(RootConstantBufferView&&) noexcept = default;
	RootConstantBufferView& operator=(const RootConstantBufferView&) noexcept = default;
	RootConstantBufferView& operator=(RootConstantBufferView&&) noexcept = default;
	~RootConstantBufferView() noexcept = default; 

	ND constexpr UINT GetRootParameterIndex() const noexcept { return m_rootParameterIndex; }
	ND constexpr ConstantBufferBase* GetConstantBuffer() const noexcept { return m_constantBuffer; }

	// No Setters yet... wait until you have a use case

	std::function<void(const Timer&, int)> Update = [](const Timer&, int) {};

private:
	UINT				m_rootParameterIndex;
	ConstantBufferBase* m_constantBuffer;
};
}