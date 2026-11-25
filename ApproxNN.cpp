#include "BallTree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

// Lower bound on the distance from q to any point inside this ball.
static double ballLowerBound(const BallNode* node, const Point& q,
                             ApproxStats* stats) {
    if (stats) ++stats->distanceComputations;
    return std::max(0.0, pointDistance(q, node->center) - node->radius);
}

static void searchANN(const BallTree& tree, const BallNode* node,
                      const Point& q, double lowerBound, double ratio,
                      int& bestId, double& bestDist, ApproxStats* stats) {
    if (!node || lowerBound > bestDist / ratio) return;
    if (stats) ++stats->nodesVisited;

    if (!node->left && !node->right) {
        const int id = node->pointIndex;
        if (stats) ++stats->distanceComputations;
        const double d = pointDistance(q, tree.points[id]);
        if (bestId < 0 || d < bestDist || (d == bestDist && id < bestId)) {
            bestId = id;
            bestDist = d;
        }
        return;
    }

    const BallNode* first = node->left;
    const BallNode* second = node->right;
    double a = first ? ballLowerBound(first, q, stats)
                     : std::numeric_limits<double>::infinity();
    double b = second ? ballLowerBound(second, q, stats)
                      : std::numeric_limits<double>::infinity();

    // Visit the closer bounding ball first, then recheck the other child.
    if (b < a) {
        std::swap(first, second);
        std::swap(a, b);
    }
    searchANN(tree, first, q, a, ratio, bestId, bestDist, stats);
    searchANN(tree, second, q, b, ratio, bestId, bestDist, stats);
}

int approximateNN(const BallTree& tree, const Point& query,
                  double eps, ApproxStats* stats) {
    if (stats) *stats = {};
    if (!std::isfinite(eps) || eps < 0.0 || !std::isfinite(1.0 + eps))
        throw std::invalid_argument("eps must be finite and nonnegative");
    if (!tree.root) return -1;
    if (query.size() != tree.points[0].size())
        throw std::invalid_argument("Query dimension mismatch");

    int bestId = -1;
    double bestDist = std::numeric_limits<double>::infinity();
    searchANN(tree, tree.root, query, 0.0, 1.0 + eps,
              bestId, bestDist, stats);
    return bestId;
}