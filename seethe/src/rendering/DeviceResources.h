#pragma once
#include "pch.h"
#include "utils/DxgiInfoManager.h"
#include "utils/TranslateErrorCode.h"


// Not sure of a use case for this because we can always just call infoManager.Set() and even if the HRESULT is a failure, but no debug messages
// are generated, we can still create a DeviceResourcesException with an empty messages vector
//#define GFX_THROW_NOINFO(hrcall) { HRESULT hr; if( FAILED( hr = (hrcall) ) ) throw DeviceResourcesException( __LINE__,__FILE__,hr ); }

#if defined(DEBUG)

#define INFOMAN seethe::DxgiInfoManager& infoManager = seethe::DeviceResources::GetInfoManager();
#define GFX_EXCEPT(hr) std::runtime_error(std::format(																				\
	"Device Resources Exception\n[Error Code] {:#x} ({})\n[Error Description]\n{}\n{}\n[File] {}\n[Line] {}\n",						\
	hr, hr, seethe::TranslateErrorCode(hr),																							\
	infoManager.GetConcatenatedMessages().empty() ? "" : std::format("[Error Info]\n{}\n", infoManager.GetConcatenatedMessages()),	\
	__FILE__, __LINE__))

#define INFO_EXCEPT() std::runtime_error(std::format(										\
	"Device Resources Info Exception\n\n[Error Info]\n{}\n\n[File] {}\n[Line] {}\n",		\
	infoManager.GetConcatenatedMessages(), __FILE__, __LINE__))

#define GFX_THROW_INFO(hrcall) { HRESULT hr; INFOMAN infoManager.Set(); if( FAILED( hr = (hrcall) ) ) throw GFX_EXCEPT(hr); }
#define GFX_THROW_INFO_ONLY(call) { INFOMAN infoManager.Set(); call; {auto v = infoManager.GetMessages(); if(!v.empty()) {throw INFO_EXCEPT();}}}

#elif defined(RELEASE)

#define INFOMAN
#define GFX_EXCEPT(hr) std::runtime_error(std::format(																\
	"Device Resources Exception\n[Error Code] {:#x} ({})\n[Error Description]\n{}\n[File] {}\n[Line] {}\n",			\
	hr, hr, seethe::TranslateErrorCode(hr),	__FILE__, __LINE__))

#define GFX_THROW_INFO(hrcall) { HRESULT hr; if( FAILED( hr = (hrcall) ) ) std::terminate(); }
#define GFX_THROW_INFO_ONLY(call) call;

#else
#error Need to define GFX exception macros
#endif






namespace seethe
{
class DeviceResources
{
public:
	DeviceResources(HWND hWnd, int height, int width); // Constructor to use when building for Win32 and we do have an HWND
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) = delete;
	DeviceResources& operator=(const DeviceResources&) = delete;
	DeviceResources& operator=(DeviceResources&&) = delete;


	void OnResize(int height, int width);
	void FlushCommandQueue();

	// Getters
	ND inline float AspectRatio() const noexcept { return static_cast<float>(m_width) / m_height; }
	ND inline ID3D12GraphicsCommandList* GetCommandList() const noexcept { return m_commandList.Get(); }
	ND inline ID3D12CommandAllocator* GetCommandAllocator() const noexcept { return m_directCmdListAlloc.Get(); }
	ND inline ID3D12CommandQueue* GetCommandQueue() const noexcept { return m_commandQueue.Get(); }
	ND inline ID3D12Device* GetDevice() const noexcept { return m_d3dDevice.Get(); }
	ND inline DXGI_FORMAT GetBackBufferFormat() const noexcept { return m_backBufferFormat; }
	ND inline DXGI_FORMAT GetDepthStencilFormat() const noexcept { return m_depthStencilFormat; }
	ND inline IDXGISwapChain1* GetSwapChain() const noexcept { return m_swapChain.Get(); }

	ND ID3D12Resource* CurrentBackBuffer() const noexcept;
	ND D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	ND D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	ND inline int GetHeight() const noexcept { return m_height; }
	ND inline int GetWidth() const noexcept { return m_width; }

	ND inline ID3D12Fence* GetFence() const noexcept { return m_fence.Get(); }
	ND inline UINT64 GetCurrentFenceValue() const noexcept { return m_currentFence; }

	ND inline UINT GetRTVDescriptorSize() const noexcept { return m_rtvDescriptorSize; }
	ND inline UINT GetDSVDescriptorSize() const noexcept { return m_dsvDescriptorSize; }
	ND inline UINT GetCBVSRVUAVDescriptorSize() const noexcept { return m_cbvSrvUavDescriptorSize; }




	void Present();

	void DelayedDelete(Microsoft::WRL::ComPtr<ID3D12Resource> resource) noexcept;
	void CleanupResources() noexcept;


private:
	void CreateDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();



	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	HWND m_hWnd;

	int m_height;
	int m_width;

	Microsoft::WRL::ComPtr<IDXGIFactory4>	m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device>	m_d3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_currentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_directCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_commandList;

	static const int SwapChainBufferCount = 2;
	int m_currBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_cbvSrvUavDescriptorSize = 0;

	D3D_DRIVER_TYPE m_d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Resources that can be deleted once they are no longer referenced by the GPU
	std::vector<std::tuple<UINT64, Microsoft::WRL::ComPtr<ID3D12Resource>>> m_resourcesToDelete;


#if defined(DEBUG)
public:
	ND inline static DxgiInfoManager& GetInfoManager() noexcept { return m_infoManager; }
private:
	static DxgiInfoManager m_infoManager;
#endif
};

}