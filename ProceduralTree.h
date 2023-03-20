#pragma once

#include "LineRenderer.h"

#include <glm/glm.hpp>

#include <vector>

class ProceduralTree {
public:
	using VertexAttributes = LineRenderer::VertexAttributes;
	
	void generate();

	std::vector<VertexAttributes> lineVertices() const { return m_lineVertices; }

private:
	std::vector<VertexAttributes> m_lineVertices;
};
