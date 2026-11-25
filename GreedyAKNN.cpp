#include "BallTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

// Simplified approximate kNN
// It is NOT the paper's two-heap, candidate-subtree kNN algorithm.
using Candidate = std::pair<double, int>;
using MaxHeap = std::priority_queue<Candidate>;

struct AKNNEntry {
    const BallNode* node;
    double centerDist;
};
struct LargerRadiusFirst {
    bool operator()(const AKNNEntry& a, const AKNNEntry& b) const {
        return a.node->radius < b.node->radius;
    }
};

static int representative(const BallTree& tree, const BallNode* node) {
    int id = node->centerIndex;
    if (id < 0 || id >= (int)tree.points.size())
        throw std::invalid_argument("Greedy node has invalid centerIndex");
    return id;
}

static void addCandidate(MaxHeap& best, int k, double d, int id) {
    Candidate candidate{d, id};
    if ((int)best.size() < k) {
        best.push(candidate);
    } else if (candidate < best.top()) {
        best.pop();
        best.push(candidate);
    }
}

std::vector<int> greedyAKNN(const BallTree& tree, const Point& query,
                            int k, double eps, ApproxStats* stats) {
    if (stats) *stats = {};
    if (!std::isfinite(eps) || eps < 0.0 || !std::isfinite(1.0 + eps))
        throw std::invalid_argument("eps must be finite and nonnegative");
    if (k <= 0) throw std::invalid_argument("k must be positive");
    if (!tree.root) return {};
    if (query.size() != tree.points[0].size())
        throw std::invalid_argument("Query dimension mismatch");
    k = std::min(k, (int)tree.points.size());
    const double ratio = 1.0 + eps;

    const int rootId = representative(tree, tree.root);
    const double rootDist = pointDistance(query, tree.points[rootId]);
    if (stats) ++stats->distanceComputations;

    MaxHeap best;
    addCandidate(best, k, rootDist, rootId);
    std::priority_queue<AKNNEntry, std::vector<AKNNEntry>, LargerRadiusFirst> todo;
    todo.push({tree.root, rootDist});

    while (!todo.empty()) {
        AKNNEntry entry = todo.top();
        todo.pop();
        const BallNode* node = entry.node;
        if ((int)best.size() == k &&
            entry.centerDist - node->radius > best.top().first / ratio)
            continue;
        if (stats) ++stats->nodesVisited;

        if (!node->left) continue;
        const BallNode* left = node->left;
        const BallNode* right = node->right;

        const double dLeft = entry.centerDist; // no new distance computation
        const int rightId = representative(tree, right);
        const double dRight = pointDistance(query, tree.points[rightId]);
        if (stats) ++stats->distanceComputations;
        addCandidate(best, k, dRight, rightId);

        const double cutoff = ((int)best.size() == k)
            ? best.top().first / ratio
            : std::numeric_limits<double>::infinity();
        if (dLeft - left->radius <= cutoff) todo.push({left, dLeft});
        if (dRight - right->radius <= cutoff) todo.push({right, dRight});
    }

    std::vector<int> result;
    while (!best.empty()) {
        result.push_back(best.top().second);
        best.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

// Convenience wrapper: specialized EXACT kNN on the same Greedy Tree.
std::vector<int> greedyKNN(const BallTree& tree, const Point& query,
                           int k, ApproxStats* stats) {
    return greedyAKNN(tree, query, k, 0.0, stats);
}