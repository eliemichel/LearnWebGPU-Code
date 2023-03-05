#include "MarchingCubesRenderer.h"
#include "ResourceManager.h"

#include <unordered_map>
#include <algorithm>

using namespace wgpu;

float quadVertices[] = {
	0.0, 0.0,
	0.0, 1.0,
	1.0, 1.0,
	1.0, 0.0
};

MarchingCubesRenderer::MarchingCubesRenderer(const InitContext& context, uint32_t resolution)
	: m_uniforms{ resolution }
	, m_device(context.device)
{
	initModuleLut(context);
	initBakingResources(context);
	initDrawingResources(context);
}

MarchingCubesRenderer::~MarchingCubesRenderer() {
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
	if (m_moduleLutBuffer) {
		m_moduleLutBuffer.destroy();
		m_moduleLutBuffer = nullptr;
	}
	if (m_texture) {
		m_texture.destroy();
		m_texture = nullptr;
	}
}

MarchingCubesRenderer::BoundComputePipeline MarchingCubesRenderer::createBoundComputePipeline(
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

struct ModuleTransform {
	using ModuleLutEntry = MarchingCubesRenderer::ModuleLutEntry;

	std::array<uint32_t, 8> permutation; // corner index -> transformed corner index

	uint32_t operator() (uint32_t moduleCode) const {
		uint32_t transformedModuleCode = 0;
		for (int i = 0; i < 8; ++i) {
			if (moduleCode & (1 << i)) {
				transformedModuleCode += 1 << permutation[i];
			}
		}
		return transformedModuleCode;
	}
	ModuleLutEntry operator() (const ModuleLutEntry& entry) const {
		return {
			permutation[entry.edgeStartCorner],
			permutation[entry.edgeEndCorner]
		};
	}
	std::vector<ModuleLutEntry> operator() (const std::vector<ModuleLutEntry>&entries) const {
		const auto& tr = *this;
		std::vector<ModuleLutEntry> transformedEntries;
		for (const auto& entry : entries) {
			transformedEntries.push_back(tr(entry));
		}
		return transformedEntries;
	}

	static glm::uvec3 CornerOffset(uint32_t i) {
		return glm::uvec3(
			(i & (1u << 0)) >> 0,
			(i & (1u << 1)) >> 1,
			(i & (1u << 2)) >> 2
		);
	}
	static uint32_t CornerIndex(const glm::uvec3& offset) {
		return (offset.x << 0) + (offset.y << 1) + (offset.z << 2);
	}

	static std::vector<ModuleTransform> ComputeAllRotations() {
		std::vector<glm::vec3> axes = {
			{1,0,0},
			{0,1,0},
			{0,0,1}
		};
		std::vector<ModuleTransform> transforms;
		// for each up axis
		for (int i = 0; i < 2 * axes.size(); ++i) {
			glm::vec3 up = axes[i % axes.size()] * (i > axes.size() ? 1.0f : -1.0f);
			glm::vec3 forward = axes[(i + 1) % axes.size()];
			// for each side face
			for (int j = 0; j < 4; ++j) {
				glm::vec3 side = cross(up, forward);
				glm::mat3 localToWorld = { forward, side, up };
				ModuleTransform tr;
				for (uint32_t k = 0; k < 8; ++k) {
					tr.permutation[k] = ModuleTransform::CornerIndex(glm::uvec3((localToWorld * (glm::vec3(ModuleTransform::CornerOffset(k)) - 0.5f)) + 0.5f));
				}
				transforms.push_back(tr);
				forward = side;
			}
		}
		return transforms;
	}
};

void MarchingCubesRenderer::initModuleLut(const InitContext& context) {
	Device device = context.device;
	Queue queue = device.getQueue();

	std::vector<ModuleTransform> transforms = ModuleTransform::ComputeAllRotations();

	// 15 base cubes that we then flip/rotate
	std::unordered_map<uint32_t,std::vector<ModuleLutEntry>> baseModules;
	{
		uint32_t c0 = ModuleTransform::CornerIndex({ 0, 0, 0 });
		uint32_t c1 = ModuleTransform::CornerIndex({ 1, 0, 0 });
		uint32_t c2 = ModuleTransform::CornerIndex({ 0, 1, 0 });
		uint32_t c3 = ModuleTransform::CornerIndex({ 0, 0, 1 });
		uint32_t moduleCode = 1u << c0;
		auto& entries = baseModules[moduleCode];
		entries.push_back({ c0, c1 });
		entries.push_back({ c0, c2 });
		entries.push_back({ c0, c3 });
	}
	{
		uint32_t c00 = ModuleTransform::CornerIndex({ 0, 0, 0 });
		uint32_t c01 = ModuleTransform::CornerIndex({ 0, 1, 0 });
		uint32_t c02 = ModuleTransform::CornerIndex({ 0, 0, 1 });
		uint32_t c10 = ModuleTransform::CornerIndex({ 1, 0, 0 });
		uint32_t c11 = ModuleTransform::CornerIndex({ 1, 1, 0 });
		uint32_t c12 = ModuleTransform::CornerIndex({ 1, 0, 1 });
		uint32_t moduleCode = (1u << c00) + (1u << c10);
		auto& entries = baseModules[moduleCode];
		entries.push_back({ c00, c01 });
		entries.push_back({ c10, c11 });
		entries.push_back({ c00, c02 });
		
		entries.push_back({ c00, c02 });
		entries.push_back({ c10, c11 });
		entries.push_back({ c10, c12 });
	}

	std::unordered_map<uint32_t, std::vector<ModuleLutEntry>> modules;
	for (const auto& cube : baseModules) {
		uint32_t moduleCode = cube.first;
		const auto& entries = cube.second;
		for (const auto& tr : transforms) {
			modules[tr(moduleCode)] = tr(entries);
		}
	}

	ModuleLut lut;
	std::fill(lut.endOffset.begin(), lut.endOffset.end(), 0);
	uint32_t offset = 0;
	for (int k = 0; k < 256; ++k) {
		auto it = modules.find(k);
		if (it != modules.end()) {
			for (auto& entry : it->second) {
				lut.entries.push_back(entry);
				++offset;
			}
		}
		lut.endOffset[k] = offset;
	}

	m_moduleLutBufferSize = static_cast<uint32_t>(256 * sizeof(uint32_t) + std::max(lut.entries.size(), (size_t)1) * sizeof(ModuleLutEntry));

	BufferDescriptor bufferDesc = Default;
	bufferDesc.label = "MarchingCubes Module LUT";
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Storage;
	bufferDesc.size = (m_moduleLutBufferSize + 3) & ~3;
	m_moduleLutBuffer = device.createBuffer(bufferDesc);
	queue.writeBuffer(m_moduleLutBuffer, 0, &lut.endOffset, 256 * sizeof(uint32_t));
	queue.writeBuffer(m_moduleLutBuffer, 256 * sizeof(uint32_t), lut.entries.data(), lut.entries.size() * sizeof(ModuleLutEntry));
}

void MarchingCubesRenderer::initBakingResources(const InitContext& context) {
	Device device = context.device;
	Queue queue = device.getQueue();

	// 1. Buffers
	BufferDescriptor bufferDesc = Default;
	bufferDesc.label = "MarchingCubes Quad";
	bufferDesc.usage = BufferUsage::Vertex;
	bufferDesc.size = sizeof(quadVertices);
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

	// 2. Texture
	TextureDescriptor textureDesc = Default;
	textureDesc.label = "MarchingCubes";
	textureDesc.dimension = TextureDimension::_3D;
	textureDesc.size = { m_uniforms.resolution, m_uniforms.resolution, m_uniforms.resolution };
	textureDesc.format = TextureFormat::RGBA16Float;
	textureDesc.mipLevelCount = 1;
	textureDesc.usage = TextureUsage::StorageBinding | TextureUsage::TextureBinding;
	m_texture = device.createTexture(textureDesc);

	TextureViewDescriptor textureViewDesc = Default;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 1;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = 1;
	textureViewDesc.dimension = TextureViewDimension::_3D;
	textureViewDesc.format = textureDesc.format;
	m_textureView = m_texture.createView(textureViewDesc);

	// 3. Bind Group Entries
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

	BindGroupLayoutEntry storageTextureBindingLayout = Default;
	storageTextureBindingLayout.binding = 1;
	storageTextureBindingLayout.visibility = ShaderStage::Compute;
	storageTextureBindingLayout.storageTexture.access = StorageTextureAccess::WriteOnly;
	storageTextureBindingLayout.storageTexture.viewDimension = TextureViewDimension::_3D;
	storageTextureBindingLayout.storageTexture.format = textureDesc.format;
	BindGroupEntry storageTextureBinding = Default;
	storageTextureBinding.binding = 1;
	storageTextureBinding.textureView = m_textureView;

	BindGroupLayoutEntry textureBindingLayout = Default;
	textureBindingLayout.binding = 2;
	textureBindingLayout.visibility = ShaderStage::Compute;
	textureBindingLayout.texture.sampleType = TextureSampleType::Float;
	textureBindingLayout.texture.viewDimension = TextureViewDimension::_3D;
	BindGroupEntry textureBinding = Default;
	textureBinding.binding = 2;
	textureBinding.textureView = m_textureView;

	BindGroupLayoutEntry countBindingLayout = Default;
	countBindingLayout.binding = 3;
	countBindingLayout.visibility = ShaderStage::Compute;
	countBindingLayout.buffer.type = BufferBindingType::Storage;
	countBindingLayout.buffer.minBindingSize = sizeof(Counts);
	BindGroupEntry countBinding = Default;
	countBinding.binding = 3;
	countBinding.buffer = m_countBuffer;
	countBinding.offset = 0;
	countBinding.size = (sizeof(Counts) + 3) & ~3;

	BindGroupLayoutEntry moduleLutBindingLayout = Default;
	moduleLutBindingLayout.binding = 4;
	moduleLutBindingLayout.visibility = ShaderStage::Compute;
	moduleLutBindingLayout.buffer.type = BufferBindingType::ReadOnlyStorage;
	moduleLutBindingLayout.buffer.minBindingSize = 0;
	BindGroupEntry moduleLutBinding = Default;
	moduleLutBinding.binding = 4;
	moduleLutBinding.buffer = m_moduleLutBuffer;
	moduleLutBinding.offset = 0;
	moduleLutBinding.size = (m_moduleLutBufferSize + 3) & ~3;

	BindGroupLayoutEntry vertexStorageBindingLayout = Default;
	vertexStorageBindingLayout.binding = 0;
	vertexStorageBindingLayout.visibility = ShaderStage::Compute;
	vertexStorageBindingLayout.buffer.type = BufferBindingType::Storage;
	vertexStorageBindingLayout.buffer.minBindingSize = 0;
	BindGroupEntry vertexStorageBinding = Default;
	vertexStorageBinding.binding = 0;
	vertexStorageBinding.buffer = m_vertexBuffer;
	vertexStorageBinding.offset = 0;
	vertexStorageBinding.size = m_vertexBufferSize;

	// This is in a separate binding group because we update it regularly
	BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
	bindGroupLayoutDesc.label = "MarchingCubes Bake Fill (group #1)";
	bindGroupLayoutDesc.entries = &vertexStorageBindingLayout;
	bindGroupLayoutDesc.entryCount = 1;
	m_vertexStorageBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	// 4. Compute pipelines
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/MarchingCubesRenderer.Bake.wgsl", device);

	m_bakingPipelines.eval = createBoundComputePipeline(
		device,
		"MarchingCubes Bake Eval",
		{ uniformBindingLayout, storageTextureBindingLayout },
		{ uniformBinding, storageTextureBinding },
		shaderModule,
		"main_eval"
	);

	m_bakingPipelines.resetCount = createBoundComputePipeline(
		device,
		"MarchingCubes Bake Reset Count",
		{ countBindingLayout },
		{ countBinding },
		shaderModule,
		"main_reset_count"
	);

	m_bakingPipelines.count = createBoundComputePipeline(
		device,
		"MarchingCubes Bake Count",
		{ uniformBindingLayout, textureBindingLayout, countBindingLayout, moduleLutBindingLayout },
		{ uniformBinding, textureBinding, countBinding, moduleLutBinding },
		shaderModule,
		"main_count"
	);

	m_bakingPipelines.fill = createBoundComputePipeline(
		device,
		"MarchingCubes Bake Fill",
		{ uniformBindingLayout, textureBindingLayout, countBindingLayout, moduleLutBindingLayout },
		{ uniformBinding, textureBinding, countBinding, moduleLutBinding },
		shaderModule,
		"main_fill",
		{ m_vertexStorageBindGroupLayout }
	);
}

void MarchingCubesRenderer::initDrawingResources(const InitContext& context) {
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
	bindGroupLayoutDesc.label = "MarchingCubes Draw";
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
	bindGroupDesc.label = "MarchingCubes Draw";
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.entryCount = static_cast<uint32_t>(bindings.size());
	bindGroupDesc.layout = bindGroupLayout;
	m_drawingBindGroup = device.createBindGroup(bindGroupDesc);

	// 2. Render pipeline
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/MarchingCubesRenderer.Draw.wgsl", device);

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

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;

	m_drawingPipeline = device.createRenderPipeline(pipelineDesc);
}

void MarchingCubesRenderer::bake() {
	Queue queue = m_device.getQueue();

	// 1. Sample distance function and count vertices
	{
		CommandEncoderDescriptor commandEncoderDesc = Default;
		commandEncoderDesc.label = "MarchingCubes Baking (Sample)";
		CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

		ComputePassDescriptor computePassDesc = Default;
		ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);

		computePass.setPipeline(m_bakingPipelines.eval.pipeline);
		computePass.setBindGroup(0, m_bakingPipelines.eval.bindGroup, 0, nullptr);
		computePass.dispatchWorkgroups(m_uniforms.resolution, m_uniforms.resolution, m_uniforms.resolution);

		computePass.setPipeline(m_bakingPipelines.resetCount.pipeline);
		computePass.setBindGroup(0, m_bakingPipelines.resetCount.bindGroup, 0, nullptr);
		computePass.dispatchWorkgroups(1, 1, 1);

		computePass.setPipeline(m_bakingPipelines.count.pipeline);
		computePass.setBindGroup(0, m_bakingPipelines.count.bindGroup, 0, nullptr);
		computePass.dispatchWorkgroups(m_uniforms.resolution - 1, m_uniforms.resolution - 1, m_uniforms.resolution - 1);

		computePass.end();

		encoder.copyBufferToBuffer(m_countBuffer, 0, m_mapBuffer, 0, sizeof(Counts));

		CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
		queue.submit(command);
	}

	// 2. Get counts
	{
		bool done = false;
		auto handle = m_mapBuffer.mapAsync(MapMode::Read, 0, sizeof(Counts), [&](BufferMapAsyncStatus status) {
			if (status == BufferMapAsyncStatus::Success) {
#ifdef WEBGPU_BACKEND_WGPU
				const Counts* countData = (const Counts*)m_mapBuffer.getMappedRange(0, sizeof(Counts));
#else
				const Counts* countData = (const Counts*)wgpuBufferGetConstMappedRange(m_mapBuffer, 0, sizeof(Counts));
#endif
				m_vertexCount = countData->pointCount;
				m_mapBuffer.unmap();
			}
			done = true;
		});

		while (!done) {
			// Do nothing, this checks for ongoing asynchronous operations and call their callbacks if needed
			wgpuQueueSubmit(queue, 0, nullptr);
		}
	}

	if (m_vertexCount == 0) {
		return;
	}

	// 3. Allocate memory
	uint32_t neededVertexBufferSize = m_vertexCount * sizeof(VertexAttributes);
	bool mustReallocate = m_vertexBufferSize < neededVertexBufferSize || neededVertexBufferSize < m_vertexBufferSize / 4;
	if (mustReallocate) {
		std::cout << "[MarchingCubesRenderer] Reallocating vertex buffer, new size = " << neededVertexBufferSize << "B" << std::endl;
		if (m_vertexBuffer) {
			m_vertexBuffer.destroy();
			m_vertexBuffer = nullptr;
			m_vertexBufferSize = 0;
		}

		BufferDescriptor bufferDesc = Default;
		bufferDesc.label = "MarchingCubes Vertices";
		bufferDesc.usage = BufferUsage::Vertex | BufferUsage::Storage;
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

	// 4. Fill in memory
	{
		CommandEncoderDescriptor commandEncoderDesc = Default;
		commandEncoderDesc.label = "MarchingCubes Baking (Fill)";
		CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

		ComputePassDescriptor computePassDesc = Default;
		ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);

		computePass.setPipeline(m_bakingPipelines.fill.pipeline);
		computePass.setBindGroup(0, m_bakingPipelines.fill.bindGroup, 0, nullptr);
		computePass.setBindGroup(1, m_vertexStorageBindGroup, 0, nullptr);
		computePass.dispatchWorkgroups(m_uniforms.resolution - 1, m_uniforms.resolution - 1, m_uniforms.resolution - 1);

		computePass.end();

		encoder.copyBufferToBuffer(m_countBuffer, 0, m_mapBuffer, 0, sizeof(Counts));

		CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
		queue.submit(command);
	}
}

void MarchingCubesRenderer::draw(const DrawingContext& context) const {
	if (!m_vertexBuffer) return;
	auto renderPass = context.renderPass;

	renderPass.setPipeline(m_drawingPipeline);

	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexCount * sizeof(VertexAttributes));
	renderPass.setBindGroup(0, m_drawingBindGroup, 0, nullptr);

	renderPass.draw(m_vertexCount, 1, 0, 0);
}
