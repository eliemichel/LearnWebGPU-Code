#include "MarchingCubesRenderer.h"
#include "ResourceManager.h"

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
	}
	if (m_texture) {
		m_texture.destroy();
		m_texture = nullptr;
	}
}

void MarchingCubesRenderer::initBakingResources(const InitContext& context) {
	auto device = context.device;

	// 1. Buffers
	BufferDescriptor bufferDesc = Default;
	bufferDesc.label = "MarchingCubes Quad";
	bufferDesc.usage = BufferUsage::Vertex;
	bufferDesc.size = sizeof(quadVertices);
	m_quadVertexBuffer = device.createBuffer(bufferDesc);

	bufferDesc.label = "MarchingCubes Uniforms";
	bufferDesc.usage = BufferUsage::Uniform;
	bufferDesc.size = (sizeof(Uniforms) + 3) & ~3;
	m_uniformBuffer = device.createBuffer(bufferDesc);

	// 2. Texture
	TextureDescriptor textureDesc = Default;
	textureDesc.label = "MarchingCubes";
	textureDesc.dimension = TextureDimension::_2D;
	textureDesc.size = { m_uniforms.resolution, m_uniforms.resolution, m_uniforms.resolution };
	textureDesc.format = TextureFormat::RGBA16Float;
	textureDesc.mipLevelCount = 1;
	textureDesc.usage = TextureUsage::RenderAttachment;// | TextureUsage::TextureBinding;
	m_texture = device.createTexture(textureDesc);

	TextureViewDescriptor textureViewDesc = Default;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = m_uniforms.resolution;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = 1;
	textureViewDesc.dimension = TextureViewDimension::_2DArray;
	textureViewDesc.format = textureDesc.format;
	m_textureView = m_texture.createView(textureViewDesc);

	// 3. Bind Group
	std::vector<BindGroupLayoutEntry> bindingLayouts(1, Default);
	bindingLayouts[0].binding = 0;
	bindingLayouts[0].visibility = ShaderStage::Fragment;
	bindingLayouts[0].buffer.type = BufferBindingType::Uniform;
	bindingLayouts[0].buffer.minBindingSize = sizeof(Uniforms);

	BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
	bindGroupLayoutDesc.label = "MarchingCubes";
	bindGroupLayoutDesc.entries = bindingLayouts.data();
	bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(bindingLayouts.size());
	BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	std::vector<BindGroupEntry> bindings(1, Default);
	bindings[0].binding = 0;
	bindings[0].buffer = m_uniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = (sizeof(Uniforms) + 3) & ~3;

	BindGroupDescriptor bindGroupDesc = Default;
	bindGroupDesc.label = "MarchingCubes";
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.entryCount = static_cast<uint32_t>(bindings.size());
	bindGroupDesc.layout = bindGroupLayout;
	m_bakingBindGroup = device.createBindGroup(bindGroupDesc);

	// 4. Compute pipelines
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/MarchingCubesRenderer.Bake.wgsl", device);

	ComputePipelineDescriptor pipelineDesc = Default;
	pipelineDesc.compute.module = shaderModule;
	PipelineLayoutDescriptor pipelineLayoutDesc = Default;
	pipelineLayoutDesc.bindGroupLayoutCount = 1;
	pipelineLayoutDesc.bindGroupLayouts = (const WGPUBindGroupLayout*)&bindGroupLayout;
	pipelineDesc.layout = device.createPipelineLayout(pipelineLayoutDesc);

	pipelineDesc.label = "MarchingCubes Bake Eval";
	pipelineDesc.compute.entryPoint = "main_eval";
	m_bakingPipelines.eval = device.createComputePipeline(pipelineDesc);

	pipelineDesc.label = "MarchingCubes Bake Count";
	pipelineDesc.compute.entryPoint = "main_count";
	m_bakingPipelines.count = device.createComputePipeline(pipelineDesc);

	pipelineDesc.label = "MarchingCubes Bake Fill";
	pipelineDesc.compute.entryPoint = "main_fill";
	m_bakingPipelines.fill = device.createComputePipeline(pipelineDesc);
}

void MarchingCubesRenderer::initDrawingResources(const InitContext& context) {
	auto device = context.device;

	// 1. Bind Group
	std::vector<BindGroupLayoutEntry> bindingLayouts(1, Default);
	bindingLayouts[0].binding = 0;
	bindingLayouts[0].visibility = ShaderStage::Fragment;
	bindingLayouts[0].buffer.type = BufferBindingType::Uniform;
	bindingLayouts[0].buffer.minBindingSize = sizeof(Uniforms);

	BindGroupLayoutDescriptor bindGroupLayoutDesc = Default;
	bindGroupLayoutDesc.label = "MarchingCubes";
	bindGroupLayoutDesc.entries = bindingLayouts.data();
	bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(bindingLayouts.size());
	BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	std::vector<BindGroupEntry> bindings(1, Default);
	bindings[0].binding = 0;
	bindings[0].buffer = m_uniformBuffer;
	bindings[0].offset = 0;
	bindings[0].size = (sizeof(Uniforms) + 3) & ~3;

	BindGroupDescriptor bindGroupDesc = Default;
	bindGroupDesc.label = "MarchingCubes";
	bindGroupDesc.entries = bindings.data();
	bindGroupDesc.entryCount = static_cast<uint32_t>(bindings.size());
	bindGroupDesc.layout = bindGroupLayout;
	m_drawingBindGroup = device.createBindGroup(bindGroupDesc);

	// 2. Render pipeline
	ShaderModule shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/MarchingCubesRenderer.Draw.wgsl", device);

	RenderPipelineDescriptor pipelineDesc = Default;

	VertexAttribute positionAttr = Default;
	positionAttr.shaderLocation = 0;
	positionAttr.format = VertexFormat::Float32x2;
	positionAttr.offset = 0;
	VertexBufferLayout vertexBufferLayout = Default;
	vertexBufferLayout.arrayStride = 2 * sizeof(float);
	vertexBufferLayout.attributeCount = 1;
	vertexBufferLayout.attributes = &positionAttr;
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;
	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";

	BlendState blend = Default;
	ColorTargetState colorTarget = Default;
	colorTarget.blend = &blend;
	colorTarget.format = context.swapChainFormat;
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

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleStrip;

	m_drawingPipeline = device.createRenderPipeline(pipelineDesc);
}

void MarchingCubesRenderer::bake() {
	Queue queue = m_device.getQueue();

	// 1. Render cube
	CommandEncoderDescriptor commandEncoderDesc = Default;
	commandEncoderDesc.label = "MarchingCubes Baking";
	CommandEncoder encoder = m_device.createCommandEncoder(commandEncoderDesc);

	ComputePassDescriptor computePassDesc = Default;
	ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);

	computePass.setPipeline(m_bakingPipelines.eval);

	computePass.setBindGroup(0, m_bakingBindGroup, 0, nullptr);

	computePass.dispatchWorkgroups(m_uniforms.resolution, m_uniforms.resolution, m_uniforms.resolution);

	computePass.end();

	CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
	queue.submit(command);

	// 2. Reset vertex data
	if (m_vertexBuffer) {
		m_vertexBuffer.destroy();
		m_vertexBuffer = nullptr;
	}

	// 3. Measure counts
	// TODO
	m_vertexCount = 0;

	if (m_vertexCount) {
		return;
	}

	// 4. Allocate memory
	BufferDescriptor bufferDesc = Default;
	bufferDesc.label = "MarchingCubes Vertices";
	bufferDesc.usage = BufferUsage::Vertex;
	bufferDesc.size = m_vertexCount * sizeof(VertexAttributes);
	m_vertexBuffer = m_device.createBuffer(bufferDesc);

	// 5. Fill in memory
	// TODO
}

void MarchingCubesRenderer::draw(const DrawingContext& context) const {
	if (true) return;
	if (!m_vertexBuffer) return;
	auto renderPass = context.renderPass;

	renderPass.setPipeline(m_drawingPipeline);

	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexCount * sizeof(VertexAttributes));
	renderPass.setBindGroup(0, m_drawingBindGroup, 0, nullptr);

	renderPass.draw(m_vertexCount, 1, 0, 0);
}
