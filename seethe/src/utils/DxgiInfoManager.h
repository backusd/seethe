#pragma once
#include "pch.h"

namespace seethe
{
class DxgiInfoManager
{
public:
	DxgiInfoManager();
	DxgiInfoManager(const DxgiInfoManager&) = delete;
	DxgiInfoManager(DxgiInfoManager&&) = delete;
	DxgiInfoManager& operator=(const DxgiInfoManager&) = delete;
	DxgiInfoManager& operator=(DxgiInfoManager&&) = delete;
	~DxgiInfoManager() noexcept;

	void Set() noexcept;
	std::vector<std::string> GetMessages() const;
	std::string GetConcatenatedMessages() const;

private:
	unsigned long long next = 0u;
	struct IDXGIInfoQueue* pDxgiInfoQueue = nullptr;
};
}