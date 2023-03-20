#include "ProceduralTree.h"

#include <random>

using glm::vec3;
using glm::vec4;

struct Parameters {
	int branchSegmentCount = 10;
	float branchSegmentRandomness = 0.1f;
	float branchSegmentLength = 0.05f;

	int firstBud = 2;
	int budPeriod = 3;
	float budRandomness = 0.3f;

	int branchLevelCount = 3;
};

// a.k.a. burgeon
struct Bud {
	vec3 position = { 0, 0, 0 };
	vec3 direction = { 0, 0, 1 };
};

void ProceduralTree::generate() {
	Parameters params;

	std::default_random_engine rnd(0);
	std::uniform_real_distribution<> branchDist(-params.branchSegmentRandomness, params.branchSegmentRandomness);
	std::uniform_real_distribution<> budDist(-params.budRandomness, params.budRandomness);

	vec4 color = { 1, 1, 1, 1 };

	std::vector<Bud> activeBuds(1);

	m_lineVertices.clear();

	for (int i = 0; i < params.branchLevelCount; ++i) {
		std::vector<Bud> nextActiveBuds;
		for (const Bud& bud : activeBuds) {
			vec3 pos = bud.position;
			vec3 direction = bud.direction;
			for (int j = 0; j < params.branchSegmentCount; ++j) {
				vec3 nextDirection = normalize(direction + vec3(branchDist(rnd), branchDist(rnd), branchDist(rnd)));
				vec3 nextPos = pos + nextDirection * params.branchSegmentLength;
				m_lineVertices.push_back({ vec4(pos, 1.0), color });
				m_lineVertices.push_back({ vec4(nextPos, 1.0), color });
				direction = nextDirection;
				pos = nextPos;

				if (j >= params.firstBud && (j - params.firstBud) % params.budPeriod == 0) {
					vec3 budDirection = normalize(direction + vec3(budDist(rnd), budDist(rnd), budDist(rnd)));
					nextActiveBuds.push_back({pos, budDirection });
				}
			}
		}
		activeBuds = nextActiveBuds;
	}
}
