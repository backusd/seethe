#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include "utils/Log.h"
#include "utils/Timer.h"
#include "utils/Constants.h"


namespace seethe
{
// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers so that we can implement the technique described by Figure 6.3.
struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT  BaseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh. 
	DirectX::BoundingBox Bounds;
};

//
// MeshGroup ======================================================================================================
//
class MeshGroup
{
public:
	MeshGroup(std::shared_ptr<DeviceResources> deviceResources) noexcept : m_deviceResources(deviceResources) {}
	MeshGroup(MeshGroup&& rhs) noexcept;
	MeshGroup& operator=(MeshGroup&& rhs) noexcept;
	virtual ~MeshGroup() noexcept {}

	inline void Bind(ID3D12GraphicsCommandList* commandList) const
	{
		GFX_THROW_INFO_ONLY(commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView));
		GFX_THROW_INFO_ONLY(commandList->IASetIndexBuffer(&m_indexBufferView));
	}

	ND inline const SubmeshGeometry& GetSubmesh(unsigned int index) const noexcept { return m_submeshes[index]; }
	virtual inline void Update(int frameIndex) noexcept {}

protected:
	ND Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize) const;
	inline void CleanUp() noexcept
	{
		// If the MeshGroup is being destructed because it was moved from, then we don't want to delete the resources
		// because they now belong to a new MeshGroup object. However, if the object simply went out of scope or was
		// intentially deleted, then the resources are no longer necessary and should be delayed deleted
		if (!m_movedFrom)
		{
			m_deviceResources->DelayedDelete(m_vertexBufferGPU);
			m_deviceResources->DelayedDelete(m_indexBufferGPU);
		}
	}

	std::shared_ptr<DeviceResources> m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;

	// Keep track of views for the two buffers
	// NOTE: All values are dummy values except the DXGI_FORMAT value for the index buffer view
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = { 0, 0, 0 };
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = { 0, 0, DXGI_FORMAT_R16_UINT };

	std::vector<SubmeshGeometry> m_submeshes;

	bool m_movedFrom = false;

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	MeshGroup(const MeshGroup&) noexcept = delete;
	MeshGroup& operator=(const MeshGroup&) noexcept = delete;
};


//
// MeshGroupT ======================================================================================================
//
template<typename T>
concept HAS_POSITION = requires(T t)
{
	{t.Position()} -> std::convertible_to<DirectX::XMFLOAT3>;
};

template<typename T>
requires HAS_POSITION<T>
class MeshGroupT : public MeshGroup
{
public:
	MeshGroupT(std::shared_ptr<DeviceResources> deviceResources,
		const std::vector<std::vector<T>>& vertices,
		const std::vector<std::vector<std::uint16_t>>& indices);
	MeshGroupT(MeshGroupT&& rhs) noexcept :
		MeshGroup(std::move(rhs)),
		m_vertices(std::move(rhs.m_vertices)),
		m_indices(std::move(rhs.m_indices))
	{
		// Specifically, see this SO post above calling std::move(rhs) but then proceding to use the rhs object: https://stackoverflow.com/questions/22977230/move-constructors-in-inheritance-hierarchy
	}
	MeshGroupT& operator=(MeshGroupT&& rhs) noexcept
	{
		// Specifically, see this SO post above calling std::move(rhs) but then proceding to use the rhs object: https://stackoverflow.com/questions/22977230/move-constructors-in-inheritance-hierarchy

		MeshGroup::operator=(std::move(rhs));
		m_vertices = std::move(rhs.m_vertices);
		m_indices = std::move(rhs.m_indices);
		return *this;
	}
	virtual ~MeshGroupT() noexcept override { CleanUp(); }

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	MeshGroupT(const MeshGroupT&) noexcept = delete;
	MeshGroupT& operator=(const MeshGroupT&) noexcept = delete;

	// System memory copies. 
	std::vector<T> m_vertices;
	std::vector<std::uint16_t> m_indices;
};

template<typename T>
requires HAS_POSITION<T>
MeshGroupT<T>::MeshGroupT(std::shared_ptr<DeviceResources> deviceResources,
	const std::vector<std::vector<T>>& vertices,
	const std::vector<std::vector<std::uint16_t>>& indices) :
	MeshGroup(deviceResources)
{
	using namespace DirectX;

	ASSERT(vertices.size() > 0, "No vertices to add");
	ASSERT(vertices.size() == indices.size(), "There must be a 1:1 correspondence between the number of vertex lists and index lists");

	// Compute the total number of vertices and indices
	size_t totalVertices = 0;
	size_t totalIndices = 0;
	for (const std::vector<T>& vec : vertices)
		totalVertices += vec.size();
	for (const std::vector<std::uint16_t>& vec : indices)
		totalIndices += vec.size();

	// reserve space for all vertices & indices
	m_vertices.reserve(totalVertices);
	m_indices.reserve(totalIndices);

	// Loop over the list of vertex lists creating a submesh for each one
	for (unsigned int iii = 0; iii < vertices.size(); ++iii)
	{
		// Create the new submesh structure for the mesh we are about to add
		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices[iii].size();
		submesh.StartIndexLocation = (UINT)m_indices.size();
		submesh.BaseVertexLocation = (INT)m_vertices.size();
		m_submeshes.push_back(submesh);

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);  
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);  

		XMVECTOR vMin = DirectX::XMLoadFloat3(&vMinf3); 
		XMVECTOR vMax = DirectX::XMLoadFloat3(&vMaxf3); 

		// Add the vertices and indices
		for (const T& v : vertices[iii])
		{
			m_vertices.push_back(v);

			// Compute the min/max for the bounding box
			XMFLOAT3 position = v.Position(); // This is guaranteed to work because we impose the HAS_POSITION concept
			XMVECTOR p = XMLoadFloat3(&position);
			vMin = XMVectorMin(vMin, p); 
			vMax = XMVectorMax(vMax, p); 
		}
		for (const std::uint16_t& i : indices[iii])
			m_indices.push_back(i);

		// Compute the bounding box
		XMStoreFloat3(&submesh.Bounds.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&submesh.Bounds.Extents, 0.5f * (vMax - vMin));
	}

	// Compute the vertex/index buffer view data
	m_vertexBufferView.StrideInBytes = sizeof(T);
	m_vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size()) * sizeof(T);
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = static_cast<UINT>(m_indices.size()) * sizeof(std::uint16_t);

	// Create the vertex and index buffers with the initial data
	m_vertexBufferGPU = CreateDefaultBuffer(m_vertices.data(), m_vertexBufferView.SizeInBytes);
	m_indexBufferGPU = CreateDefaultBuffer(m_indices.data(), m_indexBufferView.SizeInBytes);

	// Get the buffer locations
	m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
	m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
}

//
// DynamicMesh ======================================================================================================
//
// NOTE: For dynamic meshes, we make the simplification that the underlying MeshGroup will ONLY hold a single mesh.
//       The reason for this is that we have to change the vertex/index buffer view each frame as well as copy data
//		 from the CPU to GPU each frame which is all made easier by forcing the class to only manage a single Mesh
class DynamicMeshGroup : public MeshGroup
{
public:
	DynamicMeshGroup(std::shared_ptr<DeviceResources> deviceResources) : MeshGroup(deviceResources) {}
	DynamicMeshGroup(DynamicMeshGroup&& rhs) noexcept :
		MeshGroup(std::move(rhs))
	{
	}
	DynamicMeshGroup& operator=(DynamicMeshGroup&& rhs) noexcept
	{
		MeshGroup::operator=(std::move(rhs));
		return *this;
	}
	virtual ~DynamicMeshGroup() noexcept override {}
	inline void Update(int frameIndex) noexcept override
	{
		// For dynamic meshes, we keep gNumFrameResources copies of the vertex/index buffer in a single, continuous buffer
		// All we need to do every time Update() is called, is to update the vertex/index buffer views to point at the correct
		// starting location for the next buffer we want to use
		m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress() + static_cast<UINT64>(frameIndex) * m_vertexBufferView.SizeInBytes;
		m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress() + static_cast<UINT64>(frameIndex) * m_indexBufferView.SizeInBytes;
	}

protected:
	ND Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(UINT64 totalBufferSize) const;

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	DynamicMeshGroup(const DynamicMeshGroup&) = delete;
	DynamicMeshGroup& operator=(const DynamicMeshGroup&) = delete;
};


//
// MeshGroupDynamicT ======================================================================================================
//
template<typename T>
class DynamicMeshGroupT : public DynamicMeshGroup
{
public:
	// NOTE: For Dynamic meshes, we only allow there to be a single mesh - see Note above the DynamicMeshGroup class
	DynamicMeshGroupT(std::shared_ptr<DeviceResources> deviceResources,
					  std::vector<T>&& vertices,
					  std::vector<std::uint16_t>&& indices) :
		DynamicMeshGroup(deviceResources),
		m_vertices(std::move(vertices)),
		m_indices(std::move(indices))
	{
		ASSERT(m_vertices.size() > 0, "No vertices");
		ASSERT(m_indices.size() > 0, "No indices");

		// Create the submesh structure for the single mesh
		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)m_indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;
		m_submeshes.push_back(submesh);

		// Compute the vertex/index buffer view data
		m_vertexBufferView.StrideInBytes = sizeof(T);
		m_vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size()) * sizeof(T);
		m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		m_indexBufferView.SizeInBytes = static_cast<UINT>(m_indices.size()) * sizeof(std::uint16_t);

		// Create the vertex and index buffers as UPLOAD buffers (so there will be gNumFrameResources copies of the vertex/index buffers)
		m_vertexBufferGPU = CreateUploadBuffer(m_vertexBufferView.SizeInBytes);
		m_indexBufferGPU = CreateUploadBuffer(m_indexBufferView.SizeInBytes);

		// Map the vertex and index buffers
		GFX_THROW_INFO(m_vertexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedVertexData)));
		GFX_THROW_INFO(m_indexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedIndexData)));

		// Copy the data into all slots of the upload buffers (We do all slots because creation of the dynamic buffer
		// may occur at any point, not necessarily just at program start up, so we can't just assume we are on frame index 0)
		for (unsigned int iii = 0; iii < g_numFrameResources; ++iii)
		{
			memcpy(&m_mappedVertexData[iii * m_vertexBufferView.SizeInBytes], m_vertices.data(), m_vertexBufferView.SizeInBytes);
			memcpy(&m_mappedIndexData[iii * m_indexBufferView.SizeInBytes], m_indices.data(), m_indexBufferView.SizeInBytes);
		}

		// Set the buffer locations as the start of the Upload buffers. This will later be changed each frame when Update() is called
		m_vertexBufferView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
		m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	}
	DynamicMeshGroupT(DynamicMeshGroupT&& rhs) noexcept :
		DynamicMeshGroup(std::move(rhs)),
		m_vertices(std::move(rhs.m_vertices)),
		m_indices(std::move(rhs.m_indices))
	{
		// Map the vertex and index buffers
		GFX_THROW_INFO(m_vertexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedVertexData)));
		GFX_THROW_INFO(m_indexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedIndexData)));
	}
	DynamicMeshGroupT& operator=(DynamicMeshGroupT&& rhs) noexcept
	{
		DynamicMeshGroup::operator=(std::move(rhs));
		m_vertices = std::move(rhs.m_vertices);
		m_indices = std::move(rhs.m_indices);

		// Map the vertex and index buffers
		GFX_THROW_INFO(m_vertexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedVertexData)));
		GFX_THROW_INFO(m_indexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedIndexData)));

		return *this;
	}
	virtual ~DynamicMeshGroupT() noexcept override
	{
		if (m_vertexBufferGPU != nullptr)
			m_vertexBufferGPU->Unmap(0, nullptr);

		if (m_indexBufferGPU != nullptr)
			m_indexBufferGPU->Unmap(0, nullptr);

		CleanUp();
	}

	inline void CopyVertices(unsigned int frameIndex, std::vector<T>&& newVertices) noexcept
	{
		ASSERT(newVertices.size() == m_vertices.size(), "The new set of vertices must have the same total number as the original set");
		m_vertices = std::move(newVertices);

		UploadVertices(frameIndex);
	}
	inline void CopyIndices(unsigned int frameIndex, std::vector<std::uint16_t>&& newIndices) noexcept
	{
		ASSERT(newIndices.size() == m_indices.size(), "The new set of indices must have the same total number as the original set");
		m_indices = std::move(newIndices);

		UploadIndices(frameIndex);
	}

	inline void UploadVertices(unsigned int frameIndex) noexcept
	{
		ASSERT(frameIndex < g_numFrameResources, "Frame index is larger than expected");
		memcpy(&m_mappedVertexData[frameIndex * m_vertexBufferView.SizeInBytes], m_vertices.data(), m_vertexBufferView.SizeInBytes);
	}
	inline void UploadIndices(unsigned int frameIndex) noexcept
	{
		ASSERT(frameIndex < g_numFrameResources, "Frame index is larger than expected");
		memcpy(&m_mappedIndexData[frameIndex * m_indexBufferView.SizeInBytes], m_indices.data(), m_indexBufferView.SizeInBytes);
	}

	ND inline std::vector<T>& GetVertices() noexcept { return m_vertices; }
	ND inline std::vector<std::uint16_t>& GetIndices() noexcept { return m_indices; }

private:
	// There is too much state to worry about copying, so just delete copy operations until we find a good use case
	DynamicMeshGroupT(const DynamicMeshGroupT&) = delete;
	DynamicMeshGroupT& operator=(const DynamicMeshGroupT&) = delete;

	// System memory copies. 
	std::vector<T> m_vertices;
	std::vector<std::uint16_t> m_indices;

	BYTE* m_mappedVertexData = nullptr;
	BYTE* m_mappedIndexData = nullptr;
};

}