#include "GltfDebugRenderer.h"

using namespace wgpu;

void GltfDebugRenderer::create(Device device, const tinygltf::Model& model) {
	if (m_device != nullptr) destroy();
	m_device = device;
	m_device.reference(); // increase reference counter to make sure the device remains valid

	initVertexBuffer();
	initNodeData(model);
	initPipeline();
}

void GltfDebugRenderer::draw(RenderPassEncoder renderPass) {
	// Activate our pipeline dedicated to drawing frame axes
	renderPass.setPipeline(m_pipeline);
	// Activate the vertex buffer holding frame axes data
	renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexBufferByteSize);

	// Iterate over all nodes of the scene tree
	for (uint32_t i = 0; i < m_nodeCount; ++i) {
		// Draw a frame, i.e., a mesh of size 'm_vertexCount'
		renderPass.draw(m_vertexCount, 1, 0, 0);
	}
}

void GltfDebugRenderer::destroy() {
	terminatePipeline();
	terminateNodeData();
	terminateVertexBuffer();

	m_device.release(); // decrease reference counter
	m_device = nullptr;
}

void GltfDebugRenderer::initVertexBuffer() {
	static const float vertexData[] = {
		// x,  y,  z,   r,   g,   b
		0.0, 0.0, 0.0, 1.0, 0.0, 0.0, // x axis
		1.0, 0.0, 0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 1.0, 0.0, // y axis
		0.0, 1.0, 0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 1.0, // z axis
		0.0, 0.0, 1.0, 0.0, 0.0, 1.0,
	};
	BufferDescriptor bufferDesc = {};
	bufferDesc.label = "GltfDebugRenderer vertices";
	bufferDesc.mappedAtCreation = false;
	bufferDesc.size = sizeof(vertexData);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	m_vertexBuffer = m_device.createBuffer(bufferDesc);
	m_vertexBufferByteSize = bufferDesc.size;
	m_vertexCount = 5;
}

void GltfDebugRenderer::terminateVertexBuffer() {
	m_vertexBuffer.destroy();
	m_vertexBuffer.release();
	m_vertexBufferByteSize = 0;
	m_vertexCount = 0;
}

void GltfDebugRenderer::initNodeData(const tinygltf::Model& model) {
	m_nodeCount = 0;
	std::function<void(const std::vector<int>&)> visitNodes;
	visitNodes = [&](const std::vector<int>& nodeIndices) {
		for (int idx : nodeIndices) {
			const tinygltf::Node& node = model.nodes[idx];
			std::cout << " - Adding node '" << node.name << "'" << std::endl;

			// Increment node count
			++m_nodeCount;

			// Recursive call
			visitNodes(node.children);
		}
	};
	const tinygltf::Scene& scene = model.scenes[model.defaultScene];
	visitNodes(scene.nodes);
}

void GltfDebugRenderer::terminateNodeData() {
	m_nodeCount = 0;
}


void GltfDebugRenderer::initPipeline() {
#if 0 // TODO
	std::vector<BindGroupLayout> bindGroupLayouts = {
		m_bindGroupLayout,
		m_materialBindGroupLayout,
		m_nodeBindGroupLayout
	};

	// Create the pipeline layout
	PipelineLayoutDescriptor layoutDesc;
	layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
	layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)bindGroupLayouts.data();
	PipelineLayout layout = m_device.createPipelineLayout(layoutDesc);

	m_shaderModule = ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);

	RenderPipelineDescriptor pipelineDesc;
	pipelineDesc.layout = layout;

	pipelineDesc.vertex.module = m_shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::None;

	FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = m_shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	BlendState blendState;
	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;

	ColorTargetState colorTarget;
	colorTarget.format = m_swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	depthStencilState.format = m_depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Vertex fetch
	std::vector<VertexBufferLayout> vertexBufferLayouts = m_gpuScene.vertexBufferLayouts(pipelineIdx);
	pipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
	pipelineDesc.vertex.buffers = vertexBufferLayouts.data();
	pipelineDesc.primitive.topology = m_gpuScene.primitiveTopology(pipelineIdx);

	m_pipeline = m_device.createRenderPipeline(pipelineDesc);
#endif
}

void GltfDebugRenderer::terminatePipeline() {
	m_pipeline.release();
}
