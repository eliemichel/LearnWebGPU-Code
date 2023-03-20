#include "LineRenderer.h"
#include "ResourceManager.h"

#include <GLFW/glfw3.h>

#include <unordered_map>
#include <algorithm>

using namespace wgpu;

#ifdef WEBGPU_BACKEND_WGPU
#define WGPU_ANY_SIZE WGPU_WHOLE_SIZE
#else
#define WGPU_ANY_SIZE 0
#endif

const float LineRenderer::s_quadVertices[] = {
	0.0, 0.0,
	0.0, 1.0,
	1.0, 1.0,
	1.0, 0.0
};

LineRenderer::LineRenderer(const InitContext& context)
	: m_device(context.device)
{
	initBakingResources(context);
	initDrawingResources(context);
}

LineRenderer::~LineRenderer() {
	if (m_quadVertexBuffer) {
		m_quadVertexBuffer.destroy();
		m_quadVertexBuffer = nullptr;
	}
	if (m_vertexBuffer) {
		m_vertexBuffer.destroy();
		m_vertexBuffer = nullptr;
		m_vertexBufferSize = 0;
	}
	if (m_uniformBuffer) {
		m_uniformBuffer.destroy();
		m_uniformBuffer = nullptr;
	}
	if (m_countBuffer) {
		m_countBuffer.destroy();
		m_countBuffer = nullptr;
	}
	if (m_mapBuffer) {
		m_mapBuffer.destroy();
		m_mapBuffer = nullptr;
	}
}

void LineRenderer::setVertices(const std::vector<VertexAttributes>& vertexData) {
	m_vertexCount = static_cast<int32_t>(vertexData.size());
	updateVertexBufferSize();
	Queue queue = m_device.getQueue();
	queue.writeBuffer(m_vertexBuffer, 0, vertexData.data(), vertexData.size() * sizeof(VertexAttributes));
}

LineRenderer::BoundComputePipeline LineRenderer::createBoundComputePipeline(
	Device device,
	const char *label,
	const std::vector<BindGroupLayoutEntry>& bindingLayouts,
	const std::vector<BindGroupEntry>& bindings,
	ShaderModule shaderModule,
	const char* entryPoint,
	const std::vector<BindGroupLayout>& extraBindGroupLayouts
) {
	BoundComputePipeline boundPipeline;

	BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
	bindGroupLayoutDesc.label = label;
	bindGroupLayoutDesc.entries = bindingLayouts.data();
	bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(bindingLayouts.size());
	BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	BindGroupDescriptor bindGroupDesc = Default;
	bindGroupDesc.label = label;
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.entryCount = static_cast<uint32_t>(bindings.size());
	bindGroupDesc.layout = bindGroupLayout;
	boundPipeline.bindGroup = device.createBindGroup(bindGroupDesc);

	std::vector<BindGroupLayout> allBindGroupLayouts;
	allBindGroupLayouts.push_back(bindGroupLayout);
	allBindGroupLayouts.insert(allBindGroupLayouts.end(), extraBindGroupLayouts.begin(), extraBindGroupLayouts.end());

	ComputePipelineDescriptor pipelineDesc = Default;
	pipelineDesc.compute.module = shaderModule;
	PipelineLayoutDescriptor pipelineLayoutDesc = Default;
	pipelineLayoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(allBindGroupLayouts.size());
	pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)allBindGroupLayouts.data();
	pipelineDesc.layout = device.createPipelineLayout(pipelineLayoutDesc);

	pipelineDesc.label = label;
	pipelineDesc.compute.entryPoint = entryPoint;
	boundPipeline.pipeline = device.createComputePipeline(pipelineDesc);

	return boundPipeline;
}

void LineRenderer::initBakingResources(const InitContext& context) {
	Device device = context.device;
	Queue queue = device.getQueue();

	// 1. Buffers
	BufferDescriptor bufferDesc = Default;
	bufferDesc.label = "MarchingCubes Quad";
	bufferDesc.usage = BufferUsage::Vertex;
	bufferDesc.size = sizeof(s_quadVertices);
	m_quadVertexBuffer = device.createBuffer(bufferDesc);

	bufferDesc.label = "MarchingCubes Uniforms";
	bufferDesc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
	bufferDesc.size = (sizeof(Uniforms) + 3) & ~3;
	m_uniformBuffer = device.createBuffer(bufferDesc);
	queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, bufferDesc.size);

	bufferDesc.label = "MarchingCubes Counts";
	bufferDesc.usage = BufferUsage::Storage | BufferUsage::CopySrc;
	bufferDesc.size = (sizeof(Counts) + 3) & ~3;
	m_countBuffer = device.createBuffer(bufferDesc);

	bufferDesc.label = "MarchingCubes Map";
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::MapRead;
	bufferDesc.size = (sizeof(Counts) + 3) & ~3;
	m_mapBuffer = device.createBuffer(bufferDesc);

	bufferDesc.label = "MarchingCubes Vertices";
	bufferDesc.usage = BufferUsage::Vertex | BufferUsage::Storage;
	bufferDesc.size = sizeof(VertexAttributes);
	m_vertexBuffer = m_device.createBuffer(bufferDesc);
	m_vertexBufferSize = sizeof(VertexAttributes);

	// 2. Bind Group Entries
	BindGroupLayoutEntry uniformBindingLayout = Default;
	uniformBindingLayout.binding = 0;
	uniformBindingLayout.visibility = ShaderStage::Compute;
	uniformBindingLayout.buffer.type = BufferBindingType::Uniform;
	uniformBindingLayout.buffer.minBindingSize = sizeof(Uniforms);
	BindGroupEntry uniformBinding = Default;
	uniformBinding.binding = 0;
	uniformBinding.buffer = m_uniformBuffer;
	uniformBinding.offset = 0;
	uniformBinding.size = (sizeof(Uniforms) + 3) & ~3;

	BindGroupLayoutEntry countBindingLayout = Default;
	countBindingLayout.binding = 1;
	countBindingLayout.visibility = ShaderStage::Compute;
	countBindingLayout.buffer.type = BufferBindingType::Storage;
	countBindingLayout.buffer.minBindingSize = sizeof(Counts);
	BindGroupEntry countBinding = Default;
	countBinding.binding = 3;
	countBinding.buffer = m_countBuffer;
	countBinding.offset = 0;
	countBinding.size = (sizeof(Counts) + 3) & ~3;

	BindGroupLayoutEntry vertexStorageBindingLayout = Default;
	vertexStorageBindingLayout.binding = 0;
	vertexStorageBindingLayout.visibility = ShaderStage::Compute;
	vertexStorageBindingLayout.buffer.type = BufferBindingType::Storage;
	vertexStorageBindingLayout.buffer.minBindingSize = WGPU_ANY_SIZE;
	BindGroupEntry vertexStorageBinding = Default;
	vertexStorageBinding.binding = 0;
	vertexStorageBinding.buffer = m_vertexBuffer;
	vertexStorageBinding.offset = 0;
	vertexStorageBinding.size = m_vertexBufferSize;

	// This is in a separate binding group because we update it regularly
	BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
	bindGroupLayoutDesc.label = "LineRenderer Vertices (group #1)";
	bindGroupLayoutDesc.entries = &vertexStorageBindingLayout;
	bindGroupLayoutDesc.entryCount = 1;
	m_vertexStorageBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	// 3. Compute pipelines
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/LineRenderer.Bake.wgsl", device);

	m_bakingPipelines.generate = createBoundComputePipeline(
		device,
		"Procedural Tree Generate",
		{ uniformBindingLayout },
		{ uniformBinding },
		shaderModule,
		"main_generate",
		{ m_vertexStorageBindGroupLayout }
	);
}

void LineRenderer::initDrawingResources(const InitContext& context) {
	auto device = context.device;

	// 1. Bind Group
	std::vector<BindGroupLayoutEntry> bindingLayouts(2, Default);
	bindingLayouts[0].binding = 0;
	bindingLayouts[0].visibility = ShaderStage::Fragment;
	bindingLayouts[0].buffer.type = BufferBindingType::Uniform;
	bindingLayouts[0].buffer.minBindingSize = sizeof(Uniforms);

	bindingLayouts[1].binding = 1;
	bindingLayouts[1].visibility = ShaderStage::Vertex;
	bindingLayouts[1].buffer.type = BufferBindingType::Uniform;
	bindingLayouts[1].buffer.minBindingSize = context.cameraUniformBufferSize;

	BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
	bindGroupLayoutDesc.label = "Line Draw";
	bindGroupLayoutDesc.entries = bindingLayouts.data();
	bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(bindingLayouts.size());
	BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	std::vector<BindGroupEntry> bindings(2, Default);
	bindings[0].binding = 0;
	bindings[0].buffer = m_uniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = (sizeof(Uniforms) + 3) & ~3;

	bindings[1].binding = 1;
	bindings[1].buffer = context.cameraUniformBuffer;
	bindings[1].offset = 0;
	bindings[1].size = (context.cameraUniformBufferSize + 3) & ~3;

	BindGroupDescriptor bindGroupDesc = Default;
	bindGroupDesc.label = "Line Draw";
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.entryCount = static_cast<uint32_t>(bindings.size());
	bindGroupDesc.layout = bindGroupLayout;
	m_drawingBindGroup = device.createBindGroup(bindGroupDesc);

	// 2. Render pipeline
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/LineRenderer.Draw.wgsl", device);

	RenderPipelineDescriptor pipelineDesc = Default;

	std::vector<VertexAttribute> attributes(2, Default);
	VertexAttribute& positionAttr = attributes[0];
	positionAttr.shaderLocation = 0;
	positionAttr.format = VertexFormat::Float32x3;
	positionAttr.offset = 0;
	VertexAttribute& normalAttr = attributes[1];
	normalAttr.shaderLocation = 1;
	normalAttr.format = VertexFormat::Float32x3;
	normalAttr.offset = 4 * sizeof(float);
	VertexBufferLayout vertexBufferLayout = Default;
	vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	vertexBufferLayout.attributeCount = static_cast<uint32_t>(attributes.size());
	vertexBufferLayout.attributes = attributes.data();
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;
	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";

	BlendState blend = Default;
	ColorTargetState colorTarget = Default;
	colorTarget.blend = &blend;
	colorTarget.format = context.swapChainFormat;
	colorTarget.writeMask = ColorWriteMask::All;

	FragmentState fragment = Default;
	pipelineDesc.fragment = &fragment;
	fragment.module = shaderModule;
	fragment.entryPoint = "fs_main";
	fragment.targetCount = 1;
	fragment.targets = &colorTarget;

	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = context.depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;
	pipelineDesc.depthStencil = &depthStencilState;

	PipelineLayoutDescriptor pipelineLayoutDesc = Default;
	pipelineLayoutDesc.bindGroupLayoutCount = 1;
	pipelineLayoutDesc.bindGroupLayouts = (const WGPUBindGroupLayout*)&bindGroupLayout;
	pipelineDesc.layout = device.createPipelineLayout(pipelineLayoutDesc);

	pipelineDesc.primitive.topology = PrimitiveTopology::LineList;
	m_drawingPipeline = device.createRenderPipeline(pipelineDesc);
}

void LineRenderer::updateVertexBufferSize() {
	uint32_t neededVertexBufferSize = std::max(m_vertexCount, 1u) * sizeof(VertexAttributes);
	bool mustReallocate = m_vertexBufferSize < neededVertexBufferSize || neededVertexBufferSize < m_vertexBufferSize / 4;
	if (mustReallocate) {
		std::cout << "[LineRenderer] Reallocating vertex buffer, new size = " << neededVertexBufferSize << "B" << std::endl;
		if (m_vertexBuffer) {
			m_vertexBuffer.destroy();
			m_vertexBuffer = nullptr;
			m_vertexBufferSize = 0;
		}

		BufferDescriptor bufferDesc = Default;
		bufferDesc.label = "LineRenderer Vertices";
		bufferDesc.usage = BufferUsage::Vertex | BufferUsage::Storage | BufferUsage::CopyDst;
		bufferDesc.size = neededVertexBufferSize;
		m_vertexBuffer = m_device.createBuffer(bufferDesc);
		m_vertexBufferSize = neededVertexBufferSize;

		// Update vertexStorageBindGroup
		BindGroupEntry entry = Default;
		entry.binding = 0;
		entry.buffer = m_vertexBuffer;
		entry.offset = 0;
		entry.size = m_vertexBufferSize;
		BindGroupDescriptor bindGroupDesc = Default;
		bindGroupDesc.label = "MarchingCubes Bake Fill (group #1)";
		bindGroupDesc.entries = &entry;
		bindGroupDesc.entryCount = 1;
		bindGroupDesc.layout = m_vertexStorageBindGroupLayout;
		m_vertexStorageBindGroup = m_device.createBindGroup(bindGroupDesc);
	}
}

void LineRenderer::bake() {
	Queue queue = m_device.getQueue();

	m_vertexCount = 2;
	updateVertexBufferSize();

	m_uniforms.time = static_cast<float>(glfwGetTime());
	queue.writeBuffer(m_uniformBuffer, offsetof(Uniforms, time), &m_uniforms.time, sizeof(Uniforms::time));

	{
		CommandEncoderDescriptor commandEncoderDesc = Default;
		commandEncoderDesc.label = "Procedural Tree (Generate)";
		CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

		ComputePassDescriptor computePassDesc = Default;
		ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);
		
		computePass.pushDebugGroup("Tree: Generate");
		computePass.setPipeline(m_bakingPipelines.generate.pipeline);
		computePass.setBindGroup(0, m_bakingPipelines.generate.bindGroup, 0, nullptr);
		computePass.setBindGroup(1, m_vertexStorageBindGroup, 0, nullptr);
		computePass.dispatchWorkgroups(1, 1, 1);
		computePass.popDebugGroup();

		computePass.end();

		encoder.copyBufferToBuffer(m_countBuffer, 0, m_mapBuffer, 0, sizeof(Counts));

		CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
		queue.submit(command);
	}
}

void LineRenderer::draw(const DrawingContext& context) const {
	if (m_vertexCount == 0) return;
	auto renderPass = context.renderPass;

	renderPass.pushDebugGroup("Tree: Draw");

	renderPass.setPipeline(m_drawingPipeline);

	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexCount * sizeof(VertexAttributes));
	renderPass.setBindGroup(0, m_drawingBindGroup, 0, nullptr);

	renderPass.draw(m_vertexCount, 1, 0, 0);

	renderPass.popDebugGroup();
}
