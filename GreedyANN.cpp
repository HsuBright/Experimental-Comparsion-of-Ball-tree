#include "BallTree.h"

#include <cmath>
#include <queue>
#include <stdexcept>
#include <vector>

// GreedyTree.cpp invariant:
//  * Every node center is an original data point (centerIndex).
//  * For an internal node, its left child has the SAME center.
//  * Its right child's center introduces a NEW original point.
struct ANNEntry {
    const BallNode* node;
    double centerDist;  // cached distance(query, node->center)
};

struct LargerRadiusFirst {
    bool operator()(const ANNEntry& a, const ANNEntry& b) const {
        return a.node->radius < b.node->radius;
    }
};

static int representative(const BallTree& tree, const BallNode* node) {
    int id = node->centerIndex;
    if (id < 0 || id >= (int)tree.points.size())
        throw std::invalid_argument("Greedy node has invalid centerIndex");
    return id;
}

int greedyANN(const BallTree& tree, const Point& query,
              double eps, ApproxStats* stats) {
    if (stats) *stats = {};
    if (!std::isfinite(eps) || eps < 0.0 || !std::isfinite(1.0 + eps))
        throw std::invalid_argument("eps must be finite and nonnegative");
    if (!tree.root) return -1;
    if (query.size() != tree.points[0].size())
        throw std::invalid_argument("Query dimension mismatch");
    const double ratio = 1.0 + eps;

    int bestId = representative(tree, tree.root);
    double bestDist = pointDistance(query, tree.points[bestId]);
    if (stats) ++stats->distanceComputations;

    std::priority_queue<ANNEntry, std::vector<ANNEntry>, LargerRadiusFirst> todo;
    todo.push({tree.root, bestDist});

    while (!todo.empty()) {
        ANNEntry entry = todo.top();
        todo.pop();
        const BallNode* node = entry.node;
        if (entry.centerDist - node->radius > bestDist / ratio) continue;
        if (stats) ++stats->nodesVisited;

        if (!node->left) continue; // leaf: its center was already considered
        const BallNode* left = node->left;
        const BallNode* right = node->right;

        // The left child center is identical: no extra distance calculation.
        const double dLeft = entry.centerDist;
        const int rightId = representative(tree, right);
        const double dRight = pointDistance(query, tree.points[rightId]);
        if (stats) ++stats->distanceComputations;

        if (dRight < bestDist || (dRight == bestDist && rightId < bestId)) {
            bestDist = dRight;
            bestId = rightId;
        }
        if (dLeft - left->radius <= bestDist / ratio)
            todo.push({left, dLeft});
        if (dRight - right->radius <= bestDist / ratio)
            todo.push({right, dRight});
    }
    return bestId;
}

// Convenience wrapper: specialized EXACT NN on the same Greedy Tree.
int greedyNearestNeighbor(const BallTree& tree, const Point& query,
                          ApproxStats* stats) {
    return greedyANN(tree, query, 0.0, stats);
}