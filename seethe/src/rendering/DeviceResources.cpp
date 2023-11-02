#include "DeviceResources.h"
#include "utils/Log.h"
#include "utils/String.h"
//#include "utils/Profile.h"


using Microsoft::WRL::ComPtr;

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

namespace seethe
{
#if defined(DEBUG)
	DxgiInfoManager DeviceResources::m_infoManager = DxgiInfoManager();
#endif


	DeviceResources::DeviceResources(HWND hWnd, int height, int width) :
		m_hWnd(hWnd),
		m_height(height),
		m_width(width)
	{
		// PROFILE_SCOPE("DeviceResources(HWND hWnd, int height, int width)");

		CreateDevice();
		CreateCommandObjects();
		CreateRtvAndDsvDescriptorHeaps();
		CreateSwapChain();
		OnResize(m_height, m_width);
	}

	void DeviceResources::Set4xMsaaState(bool value)
	{
		if (m_4xMsaaState != value)
		{
			m_4xMsaaState = value;

			// Recreate the swapchain and buffers with new multisample settings.
			CreateSwapChain();
		}
	}

	void DeviceResources::CreateDevice()
	{
#if defined(DEBUG) || defined(_DEBUG) 
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			GFX_THROW_INFO(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
#endif

	GFX_THROW_INFO(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_d3dDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		LOG_WARN("{}", "Failed to create D3D12 device. Attempting to fallback to WARP device...");

		ComPtr<IDXGIAdapter> pWarpAdapter;
		GFX_THROW_INFO(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		GFX_THROW_INFO(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_d3dDevice)));
	}

	GFX_THROW_INFO(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
	msQualityLevels.Format = m_backBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	GFX_THROW_INFO(
		m_d3dDevice->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&msQualityLevels,
			sizeof(msQualityLevels)
		)
	);

	m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif
	}

	void DeviceResources::CreateCommandObjects()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		GFX_THROW_INFO(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		GFX_THROW_INFO(
			m_d3dDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_directCmdListAlloc.GetAddressOf())
			)
		);

		GFX_THROW_INFO(
			m_d3dDevice->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				m_directCmdListAlloc.Get(), // Associated command allocator
				nullptr,                   // Initial PipelineStateObject
				IID_PPV_ARGS(m_commandList.GetAddressOf())
			)
		);

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		m_commandList->Close();
	}
	void DeviceResources::CreateRtvAndDsvDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		GFX_THROW_INFO(
			m_d3dDevice->CreateDescriptorHeap(
				&rtvHeapDesc,
				IID_PPV_ARGS(m_rtvHeap.GetAddressOf())
			)
		);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		GFX_THROW_INFO(
			m_d3dDevice->CreateDescriptorHeap(
				&dsvHeapDesc,
				IID_PPV_ARGS(m_dsvHeap.GetAddressOf())
			)
		);
	}
	void DeviceResources::CreateSwapChain()
	{
		// Release the previous swapchain we will be recreating.
		m_swapChain.Reset();

		// If we are using an HWND create the SwapChainDesc for a specific window
		// otherwise we are creating it for composition (UWP)
		if (m_hWnd != 0)
		{
			DXGI_SWAP_CHAIN_DESC sd = {};
			sd.BufferDesc.Width = m_width;
			sd.BufferDesc.Height = m_height;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferDesc.Format = m_backBufferFormat;
			sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			sd.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
			sd.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = SwapChainBufferCount;
			sd.OutputWindow = m_hWnd;
			sd.Windowed = true;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

			// Note: Swap chain uses queue to perform flush.
			ComPtr<IDXGISwapChain> swapChain = nullptr;
			GFX_THROW_INFO(
				m_dxgiFactory->CreateSwapChain(
					m_commandQueue.Get(),
					&sd,
					swapChain.ReleaseAndGetAddressOf()
				)
			);

			// Get interface for IDXGISwapChain1
			swapChain.As(&m_swapChain);
		}
		else
		{
			// Otherwise, create a new one using the same adapter as the existing Direct3D device.
			DXGI_SWAP_CHAIN_DESC1 sd = { 0 };

			sd.Width = m_width; // Match the size of the window.
			sd.Height = m_height;
			sd.Format = m_backBufferFormat; // This is the most common swap chain format.
			sd.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
			sd.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = SwapChainBufferCount;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // MUST use this swap effect when creating swapchain for composition
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sd.Scaling = DXGI_SCALING_STRETCH;				  // MUST use this scaling behavior when creating swapchain for composition
			sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

			GFX_THROW_INFO(
				m_dxgiFactory->CreateSwapChainForComposition(
					m_commandQueue.Get(),
					&sd,
					nullptr,
					m_swapChain.ReleaseAndGetAddressOf()
				)
			);
		}
	}


	void DeviceResources::FlushCommandQueue()
	{
		// Advance the fence value to mark commands up to this fence point.
		m_currentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		GFX_THROW_INFO(m_commandQueue->Signal(m_fence.Get(), m_currentFence));

		// Wait until the GPU has completed commands up to this fence point.
		if (m_fence->GetCompletedValue() < m_currentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			GFX_THROW_INFO(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
#pragma warning(suppress : 6387)
			WaitForSingleObject(eventHandle, INFINITE);
#pragma warning(suppress : 6387)
			CloseHandle(eventHandle);
		}
	}

	ID3D12Resource* DeviceResources::CurrentBackBuffer() const noexcept
	{
		return m_swapChainBuffer[m_currBackBuffer].Get();
	}
	D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::CurrentBackBufferView() const noexcept
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_currBackBuffer,
			m_rtvDescriptorSize
		);
	}
	D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::DepthStencilView() const noexcept
	{
		return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	void DeviceResources::OnResize(int height, int width)
	{
		ASSERT(m_d3dDevice != nullptr, "device is null");
		ASSERT(m_swapChain != nullptr, "swapchain is null");
		ASSERT(m_directCmdListAlloc != nullptr, "command list allocator is null");

		m_height = height;
		m_width = width;

		// Flush before changing any resources.
		FlushCommandQueue();

		GFX_THROW_INFO(m_commandList->Reset(m_directCmdListAlloc.Get(), nullptr));

		// Release the previous resources we will be recreating.
		for (int i = 0; i < SwapChainBufferCount; ++i)
			m_swapChainBuffer[i].Reset();
		m_depthStencilBuffer.Reset();

		// Resize the swap chain.
		GFX_THROW_INFO(
			m_swapChain->ResizeBuffers(
				SwapChainBufferCount,
				m_width,
				m_height,
				m_backBufferFormat,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
			)
		);

		m_currBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			GFX_THROW_INFO(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));
			m_d3dDevice->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
		}

		// Create the depth/stencil buffer and view.
		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = m_width;
		depthStencilDesc.Height = m_height;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;

		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
		depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear = {};
		optClear.Format = m_depthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		auto _p = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		GFX_THROW_INFO(
			m_d3dDevice->CreateCommittedResource(
				&_p,
				D3D12_HEAP_FLAG_NONE,
				&depthStencilDesc,
				D3D12_RESOURCE_STATE_COMMON,
				&optClear,
				IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())
			)
		);

		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = m_depthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		m_d3dDevice->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

		// Transition the resource from its initial state to be used as a depth buffer.
		auto _b = CD3DX12_RESOURCE_BARRIER::Transition(
			m_depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
		m_commandList->ResourceBarrier(1, &_b);

		// Execute the resize commands.
		GFX_THROW_INFO(m_commandList->Close());
		ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until resize is complete.
		FlushCommandQueue();
	}

	void DeviceResources::LogAdapters()
	{
		UINT i = 0;
		IDXGIAdapter* adapter = nullptr;
		std::vector<IDXGIAdapter*> adapterList;
		while (m_dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			std::wstring text = L"***Adapter: ";
			text += desc.Description;

			LOG_INFO("{}", ws2s(text)); // std::string(text.begin(), text.end()));

			adapterList.push_back(adapter);

			++i;
		}

		for (size_t i = 0; i < adapterList.size(); ++i)
		{
			LogAdapterOutputs(adapterList[i]);
			ReleaseCom(adapterList[i]);
		}
	}
	void DeviceResources::LogAdapterOutputs(IDXGIAdapter* adapter)
	{
		UINT i = 0;
		IDXGIOutput* output = nullptr;
		while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_OUTPUT_DESC desc;
			output->GetDesc(&desc);

			std::wstring text = L"***Output: ";
			text += desc.DeviceName;

			LOG_INFO("{}", ws2s(text));

			LogOutputDisplayModes(output, m_backBufferFormat);

			ReleaseCom(output);

			++i;
		}
	}
	void DeviceResources::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
	{
		UINT count = 0;
		UINT flags = 0;

		// Call with nullptr to get list count.
		output->GetDisplayModeList(format, flags, &count, nullptr);

		std::vector<DXGI_MODE_DESC> modeList(count);
		output->GetDisplayModeList(format, flags, &count, &modeList[0]);

		for (auto& x : modeList)
		{
			UINT n = x.RefreshRate.Numerator;
			UINT d = x.RefreshRate.Denominator;
			std::wstring text =
				L"Width = " + std::to_wstring(x.Width) + L" " +
				L"Height = " + std::to_wstring(x.Height) + L" " +
				L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d);

			LOG_INFO("{}", ws2s(text));
		}
	}


	void DeviceResources::Present()
	{
		// PROFILE_SCOPE("m_swapChain->Present()");

		// swap the back and front buffers
		GFX_THROW_INFO(m_swapChain->Present(0, 0));
		m_currBackBuffer = (m_currBackBuffer + 1) % SwapChainBufferCount;

		// Wait until frame commands are complete.  This waiting is inefficient and is
		// done for simplicity.  Later we will show how to organize our rendering code
		// so we do not have to wait per frame.
		//FlushCommandQueue();


		++m_currentFence;
	}

	void DeviceResources::DelayedDelete(Microsoft::WRL::ComPtr<ID3D12Resource> resource) noexcept
	{
		// store a ComPtr to the resource as well as the current fence value which is the maximum fence
		// value where the resource might still be referenced on the GPU
		m_resourcesToDelete.emplace_back(m_currentFence, resource);
	}

	void DeviceResources::CleanupResources() noexcept
	{
		if (m_resourcesToDelete.size() > 0)
		{
			UINT64 completedFence = m_fence->GetCompletedValue();

			// Erase all elements for which the completed fence is at or beyond the fence value in the tuple
			std::erase_if(m_resourcesToDelete, [&completedFence](const std::tuple<UINT64, Microsoft::WRL::ComPtr<ID3D12Resource>>& tup) 
				{
					return completedFence >= std::get<0>(tup); 
				}
			);
		}
	}
}