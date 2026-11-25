#include "BallTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

// Largest (distance, id) is the worst candidate, at the top.
using Candidate = std::pair<double, int>;
using MaxHeap = std::priority_queue<Candidate>;

static double ballLowerBound(const BallNode* node, const Point& q,
                             ApproxStats* stats) {
    if (stats) ++stats->distanceComputations;
    return std::max(0.0, pointDistance(q, node->center) - node->radius);
}

static void searchAKNN(const BallTree& tree, const BallNode* node,
                       const Point& q, double lowerBound, double ratio,
                       int k, MaxHeap& best, ApproxStats* stats) {
    if (!node) return;
    // Before finding k candidates, no distance-based pruning is allowed.
    if ((int)best.size() == k && lowerBound > best.top().first / ratio)
        return;
    if (stats) ++stats->nodesVisited;

    if (!node->left && !node->right) {
        const int id = node->pointIndex;
        if (stats) ++stats->distanceComputations;
        Candidate c{pointDistance(q, tree.points[id]), id};
        if ((int)best.size() < k) {
            best.push(c);
        } else if (c < best.top()) {
            best.pop();
            best.push(c);
        }
        return;
    }

    const BallNode* first = node->left;
    const BallNode* second = node->right;
    double a = first ? ballLowerBound(first, q, stats)
                     : std::numeric_limits<double>::infinity();
    double b = second ? ballLowerBound(second, q, stats)
                      : std::numeric_limits<double>::infinity();
    if (b < a) {
        std::swap(first, second);
        std::swap(a, b);
    }
    searchAKNN(tree, first, q, a, ratio, k, best, stats);
    searchAKNN(tree, second, q, b, ratio, k, best, stats);
}

std::vector<int> approximateKNN(const BallTree& tree, const Point& query,
                                int k, double eps, ApproxStats* stats) {
    if (stats) *stats = {};
    if (!std::isfinite(eps) || eps < 0.0 || !std::isfinite(1.0 + eps))
        throw std::invalid_argument("eps must be finite and nonnegative");
    if (k <= 0) throw std::invalid_argument("k must be positive");
    if (!tree.root) return {};
    if (query.size() != tree.points[0].size())
        throw std::invalid_argument("Query dimension mismatch");

    k = std::min(k, (int)tree.points.size());
    MaxHeap best;
    searchAKNN(tree, tree.root, query, 0.0, 1.0 + eps, k, best, stats);

    std::vector<int> result;
    while (!best.empty()) {
        result.push_back(best.top().second);
        best.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}