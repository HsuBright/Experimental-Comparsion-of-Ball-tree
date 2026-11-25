#ifndef BALLTREE_H
#define BALLTREE_H

#include "BallNode.h"
#include <cassert>
#include <stdexcept>
#include <vector>

struct BallTree {
    std::vector<Point> points;
    std::vector<BallNode> nodes;

    BallNode* root = nullptr;
    int used = 0;
};

// Initialize the tree
inline void initTree(
    BallTree& tree,
    const std::vector<Point>& data
) {
    if (data.empty() || data[0].empty()) {
        throw std::invalid_argument(
            "Dataset must be nonempty"
        );
    }

    for (const Point& p : data) {
        if (p.size() != data[0].size()) {
            throw std::invalid_argument(
                "Point dimensions must match"
            );
        }
    }

    tree.root = nullptr;
    tree.used = 0;
    tree.points = data;

    // Preallocate all nodes
    tree.nodes.clear();
    tree.nodes.resize(2 * data.size() - 1);
}

// Get a node from the node pool
inline BallNode* newNode(BallTree& tree) {
    assert(tree.used < static_cast<int>(
        tree.nodes.size()
    ));

    return &tree.nodes[tree.used++];
}

// Construction algorithms
void buildMedian(BallTree& tree,
                 const std::vector<Point>& data);

void buildTopDown(BallTree& tree,
                  const std::vector<Point>& data);

void buildFarthestPivot(BallTree& tree,
                        const std::vector<Point>& data);

void buildGreedy(BallTree& tree,
                 const std::vector<Point>& data);

// Exact search algorithms
int nearestNeighbor(const BallTree& tree,
                    const Point& query);

std::vector<int> kNN(const BallTree& tree,
                     const Point& query,
                     int k);

std::vector<int> rangeSearch(const BallTree& tree,
                             const Point& query,
                             double radius);

// Search statistics
struct ApproxStats {
    long long nodesVisited = 0;
    long long distanceComputations = 0;
};

// Generic approximate search (all four trees)
int approximateNN(
    const BallTree& tree,
    const Point& query,
    double eps,
    ApproxStats* stats = nullptr
);

std::vector<int> approximateKNN(
    const BallTree& tree,
    const Point& query,
    int k,
    double eps,
    ApproxStats* stats = nullptr
);

// Greedy Tree specialized approximate search
int greedyANN(
    const BallTree& tree,
    const Point& query,
    double eps,
    ApproxStats* stats = nullptr
);

std::vector<int> greedyAKNN(
    const BallTree& tree,
    const Point& query,
    int k,
    double eps,
    ApproxStats* stats = nullptr
);




#endif
