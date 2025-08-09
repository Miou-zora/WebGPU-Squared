#include "Mesh.hpp"

namespace ES::Plugin::WebGPU::Component {

Mesh::Mesh(ES::Engine::Core &core, const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals, const std::vector<glm::vec2> &uvs, const std::vector<uint32_t> &indices) {
	auto &device = core.GetResource<wgpu::Device>();
	auto &queue = core.GetResource<wgpu::Queue>();

	std::vector<float> pointData;
	for (size_t i = 0; i < vertices.size(); i++) {
		pointData.push_back(vertices.at(i).x);
		pointData.push_back(vertices.at(i).y);
		pointData.push_back(vertices.at(i).z);
		pointData.push_back(normals.at(i).r);
		pointData.push_back(normals.at(i).g);
		pointData.push_back(normals.at(i).b);
		pointData.push_back(uvs.at(i).x);
		pointData.push_back(uvs.at(i).y);
	}


	std::vector<uint32_t> indexData;
	for (size_t i = 0; i < indices.size(); i++) {
		indexData.push_back(indices.at(i));
	}

	indexCount = indexData.size();

	wgpu::BufferDescriptor bufferDesc(wgpu::Default);
	bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
	pointBuffer = device.createBuffer(bufferDesc);

	queue.writeBuffer(pointBuffer, 0, pointData.data(), bufferDesc.size);

	bufferDesc.size = indexData.size() * sizeof(uint32_t);
	bufferDesc.size = (bufferDesc.size + 3) & ~3;
	indexData.resize((indexData.size() + 1) & ~1);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
	indexBuffer = device.createBuffer(bufferDesc);

	queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);
}

void Mesh::Release() {
	if (pointBuffer) {
		pointBuffer.destroy();
		pointBuffer.release();
		pointBuffer = nullptr;
	}
	if (indexBuffer) {
		indexBuffer.destroy();
		indexBuffer.release();
		indexBuffer = nullptr;
	}
	indexCount = 0;
}

}
