#include "BallTree.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

static int height(const BallNode* node) {
    if (!node) return 0;
    return 1 + std::max(height(node->left), height(node->right));
}

// Brute-force reference: sorted by (distance, original point ID).
static std::vector<int> bruteKNN(const std::vector<Point>& points,
                                  const Point& query, int k) {
    std::vector<int> ids(points.size());
    std::iota(ids.begin(), ids.end(), 0);
    std::partial_sort(ids.begin(), ids.begin() + k, ids.end(),
        [&](int a, int b) {
            const double da = pointDistance(points[a], query);
            const double db = pointDistance(points[b], query);
            return da == db ? a < b : da < db;
        });
    ids.resize(k);
    return ids;
}

static std::vector<Point> makeData(int n, int dim, const std::string& dataset,
                                   std::mt19937_64& rng,
                                   const std::vector<Point>& centers) {
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    std::normal_distribution<double> normal(0.0, 0.06);
    std::vector<Point> data(n, Point(dim));
    const int uniqueCount = std::max(1, n / 8);
    for (int i = 0; i < n; ++i) {
        if (dataset == "duplicates" && i >= uniqueCount) {
            data[i] = data[i % uniqueCount];
        } else {
            for (int j = 0; j < dim; ++j) {
                data[i][j] = dataset == "clusters"
                    ? centers[i % (int)centers.size()][j] + normal(rng)
                    : uni(rng);
            }
        }
    }
    return data;
}

static std::vector<Point> makeQueries(int count, int dim,
                                      const std::string& dataset,
                                      const std::vector<Point>& points,
                                      std::mt19937_64& rng,
                                      const std::vector<Point>& centers) {
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    std::normal_distribution<double> normal(0.0, 0.06);
    std::uniform_int_distribution<int> pick(0, (int)points.size() - 1);
    std::vector<Point> queries(count, Point(dim));
    for (int i = 0; i < count; ++i) {
        if (dataset == "duplicates" && i % 2 == 0) {
            queries[i] = points[pick(rng)];
        } else {
            for (int j = 0; j < dim; ++j) {
                queries[i][j] = dataset == "clusters"
                    ? centers[i % (int)centers.size()][j] + normal(rng)
                    : uni(rng);
            }
        }
    }
    return queries;
}

struct Measurement {
    double avgQueryUs = 0.0;
    std::uint64_t checksum = 0;
    double avgNodesVisited = -1.0;
    double avgDistanceComputations = -1.0;
};

// The lambda is compiled for each query type; no std::function overhead.
template <typename F>
static Measurement measureNN(const std::vector<Point>& queries,
                             bool hasStats, F call) {
    volatile std::uint64_t checksum = 0;
    std::uint64_t visits = 0, computations = 0;
    const auto start = Clock::now();
    for (const Point& q : queries) {
        ApproxStats stat;
        const int id = call(q, hasStats ? &stat : nullptr);
        checksum = checksum + (std::uint64_t)id + 1;
        visits += (std::uint64_t)stat.nodesVisited;
        computations += (std::uint64_t)stat.distanceComputations;
    }
    const auto stop = Clock::now();
    Measurement out;
    out.avgQueryUs = 1e6 * std::chrono::duration<double>(stop - start).count()
        / queries.size();
    out.checksum = checksum;
    if (hasStats) {
        out.avgNodesVisited = (double)visits / queries.size();
        out.avgDistanceComputations = (double)computations / queries.size();
    }
    return out;
}

template <typename F>
static Measurement measureKNN(const std::vector<Point>& queries,
                              bool hasStats, F call) {
    volatile std::uint64_t checksum = 0;
    std::uint64_t visits = 0, computations = 0;
    const auto start = Clock::now();
    for (const Point& q : queries) {
        ApproxStats stat;
        const std::vector<int> ids = call(q, hasStats ? &stat : nullptr);
        for (int id : ids) checksum = checksum + (std::uint64_t)id + 1;
        visits += (std::uint64_t)stat.nodesVisited;
        computations += (std::uint64_t)stat.distanceComputations;
    }
    const auto stop = Clock::now();
    Measurement out;
    out.avgQueryUs = 1e6 * std::chrono::duration<double>(stop - start).count()
        / queries.size();
    out.checksum = checksum;
    if (hasStats) {
        out.avgNodesVisited = (double)visits / queries.size();
        out.avgDistanceComputations = (double)computations / queries.size();
    }
    return out;
}

struct Quality {
    double meanDistanceRatio = 0.0;
    double maxDistanceRatio = 0.0;
    double recall = 0.0;   // index-overlap with exact kNN, tie-sensitive
};

static double ratioOf(double returnedDist, double trueDist) {
    if (trueDist == 0.0)
        return returnedDist == 0.0 ? 1.0 : std::numeric_limits<double>::infinity();
    return returnedDist / trueDist;
}

static bool withinBound(double obtained, double truth, double eps) {
    const double tolerance = 1e-10 * std::max(1.0, truth);
    return obtained <= (1.0 + eps) * truth + tolerance;
}

template <typename F>
static Quality verifyNN(const std::vector<Point>& points,
                        const std::vector<Point>& queries,
                        const std::vector<std::vector<int>>& truth,
                        double eps, bool requireExact, const std::string& label,
                        F call) {
    Quality output;
    const int checks = (int)truth.size();
    for (int i = 0; i < checks; ++i) {
        const int got = call(queries[i], nullptr);
        if (got < 0 || got >= (int)points.size())
            throw std::runtime_error(label + ": invalid NN index");
        const int trueID = truth[i][0];
        const double gotD = pointDistance(points[got], queries[i]);
        const double trueD = pointDistance(points[trueID], queries[i]);
        if (!withinBound(gotD, trueD, requireExact ? 0.0 : eps)) {
            throw std::runtime_error(label + ": NN correctness failed at query " +
                                     std::to_string(i));
        }
        const double ratio = ratioOf(gotD, trueD);
        output.meanDistanceRatio += ratio;
        output.maxDistanceRatio = std::max(output.maxDistanceRatio, ratio);
        output.recall += (got == trueID ? 1.0 : 0.0);
    }
    output.meanDistanceRatio /= checks;
    output.recall /= checks;
    return output;
}

template <typename F>
static Quality verifyKNN(const std::vector<Point>& points,
                         const std::vector<Point>& queries,
                         const std::vector<std::vector<int>>& truth,
                         int k, double eps, bool requireExact,
                         const std::string& label, F call) {
    Quality output;
    const int checks = (int)truth.size();
    for (int i = 0; i < checks; ++i) {
        const std::vector<int> got = call(queries[i], nullptr);
        if ((int)got.size() != k)
            throw std::runtime_error(label + ": incorrect number of kNN results");
        std::vector<int> uniq = got;
        std::sort(uniq.begin(), uniq.end());
        if (std::adjacent_find(uniq.begin(), uniq.end()) != uniq.end() ||
            uniq.front() < 0 || uniq.back() >= (int)points.size())
            throw std::runtime_error(label + ": duplicate or invalid kNN result IDs");
        const double gotK = pointDistance(points[got.back()], queries[i]);
        const double trueK = pointDistance(points[truth[i].back()], queries[i]);
        if (!withinBound(gotK, trueK, requireExact ? 0.0 : eps)) {
            throw std::runtime_error(label + ": kNN correctness failed at query " +
                                     std::to_string(i));
        }
        // For EXACT kNN check all k sorted distances. Equal-distance
        // duplicates can legitimately have different original IDs.
        if (requireExact) {
            for (int j = 0; j < k; ++j) {
                const double candidateD = pointDistance(points[got[j]], queries[i]);
                const double referenceD = pointDistance(points[truth[i][j]], queries[i]);
                if (!withinBound(candidateD, referenceD, 0.0))
                    throw std::runtime_error(label + ": wrong exact kNN distance at query " +
                                             std::to_string(i));
            }
        }
        int match = 0;
        for (int id : got)
            if (std::find(truth[i].begin(), truth[i].end(), id) != truth[i].end())
                ++match;
        const double ratio = ratioOf(gotK, trueK);
        output.meanDistanceRatio += ratio;
        output.maxDistanceRatio = std::max(output.maxDistanceRatio, ratio);
        output.recall += (double)match / k;
    }
    output.meanDistanceRatio /= checks;
    output.recall /= checks;
    return output;
}

static void printRow(const std::string& dataset, int n, int dim,
                     int queries, int k, int seed, double eps,
                     const std::string& method, const char* searchImpl,
                     const char* queryType, double buildMs, int treeHeight,
                     const Measurement& m, const Quality& q) {
    std::cout << dataset << ',' << n << ',' << dim << ',' << queries
              << ',' << k << ',' << seed << ',' << eps << ',' << method
              << ',' << searchImpl << ',' << queryType
              << ',' << buildMs << ',' << m.avgQueryUs << ',' << treeHeight
              << ",PASS," << m.checksum << ',' << m.avgNodesVisited
              << ',' << m.avgDistanceComputations << ',' << q.meanDistanceRatio
              << ',' << q.maxDistanceRatio << ',' << q.recall << '\n';
}

static void benchmarkMethod(const std::string& method,
                            const std::string& dataset,
                            const std::vector<Point>& points,
                            const std::vector<Point>& queries,
                            const std::vector<std::vector<int>>& truth,
                            int k, int seed, double eps) {
    BallTree tree;
    const auto start = Clock::now();
    if (method == "Median") buildMedian(tree, points);
    else if (method == "TopDown") buildTopDown(tree, points);
    else if (method == "FarthestPivot") buildFarthestPivot(tree, points);
    else if (method == "Greedy") buildGreedy(tree, points);
    else throw std::invalid_argument("Unknown tree method");
    const auto stop = Clock::now();
    const double buildMs = std::chrono::duration<double, std::milli>(stop - start).count();
    const int h = height(tree.root);
    const int n = (int)points.size();
    const int dim = (int)points[0].size();

    // All four trees: generic EXACT and generic APPROXIMATE searches.
    const auto exactNN = [&](const Point& q, ApproxStats*) {
        return nearestNeighbor(tree, q);
    };
    const auto exactKNN = [&](const Point& q, ApproxStats*) {
        return kNN(tree, q, k);
    };
    const auto ann = [&](const Point& q, ApproxStats* s) {
        return approximateNN(tree, q, eps, s);
    };
    const auto aknn = [&](const Point& q, ApproxStats* s) {
        return approximateKNN(tree, q, k, eps, s);
    };

    auto emitNN = [&](const char* impl, const char* type,
                      bool hasStats, bool requireExact, auto call) {
        const Measurement m = measureNN(queries, hasStats, call);
        const Quality q = verifyNN(points, queries, truth, eps,
                                   requireExact, method + "/" + impl + "/" + type, call);
        printRow(dataset, n, dim, (int)queries.size(), k, seed, eps,
                 method, impl, type, buildMs, h, m, q);
    };
    auto emitKNN = [&](const char* impl, const char* type,
                       bool hasStats, bool requireExact, auto call) {
        const Measurement m = measureKNN(queries, hasStats, call);
        const Quality q = verifyKNN(points, queries, truth, k, eps,
                                    requireExact, method + "/" + impl + "/" + type, call);
        printRow(dataset, n, dim, (int)queries.size(), k, seed, eps,
                 method, impl, type, buildMs, h, m, q);
    };

    emitNN("Generic", "NN", false, true, exactNN);
    emitKNN("Generic", "kNN", false, true, exactKNN);
    emitNN("Generic", "ANN", true, eps == 0.0, ann);
    emitKNN("Generic", "AkNN", true, eps == 0.0, aknn);

    // Extra Greedy-only exact and approximate searches.
    if (method == "Greedy") {
        const auto gANN = [&](const Point& q, ApproxStats* s) {
            return greedyANN(tree, q, eps, s);
        };
        const auto gAKNN = [&](const Point& q, ApproxStats* s) {
            return greedyAKNN(tree, q, k, eps, s);
        };
        emitNN("GreedySpecialized", "ANN", true, eps == 0.0, gANN);
        emitKNN("GreedySpecialized", "AkNN", true, eps == 0.0, gAKNN);
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 7 && argc != 8) {
            std::cerr << "Usage: " << argv[0]
                      << " N DIM QUERIES K SEED uniform|clusters|duplicates [EPS]\n";
            return 2;
        }
        const int n = std::stoi(argv[1]);
        const int dim = std::stoi(argv[2]);
        const int queryCount = std::stoi(argv[3]);
        const int k = std::stoi(argv[4]);
        const int seed = std::stoi(argv[5]);
        const std::string dataset = argv[6];
        const double eps = (argc == 8) ? std::stod(argv[7]) : 0.25;
        if (n < 1 || dim < 1 || queryCount < 1 || k < 1 || k > n ||
            !std::isfinite(eps) || eps < 0.0 || !std::isfinite(1.0 + eps) ||
            (dataset != "uniform" && dataset != "clusters" && dataset != "duplicates"))
            throw std::invalid_argument("Invalid benchmark parameters");

        std::mt19937_64 rng((std::uint64_t)seed);
        std::uniform_real_distribution<double> uni(0.0, 1.0);
        std::vector<Point> centers(4, Point(dim));
        for (Point& c : centers)
            for (double& x : c) x = uni(rng);
        const std::vector<Point> points = makeData(n, dim, dataset, rng, centers);
        const std::vector<Point> queries = makeQueries(
            queryCount, dim, dataset, points, rng, centers);

        // Precompute EXACT reference answers OUTSIDE all timing windows.
        const int checks = std::min(queryCount, 12);
        std::vector<std::vector<int>> truth;
        truth.reserve(checks);
        for (int i = 0; i < checks; ++i)
            truth.push_back(bruteKNN(points, queries[i], k));

        std::cout << std::fixed << std::setprecision(7);
        std::cout << "dataset,n,dim,queries,k,seed,eps,method,search_impl,query_type,"
                     "build_ms,avg_query_us,tree_height,correct,checksum,"
                     "avg_nodes_visited,avg_distance_computations,"
                     "sample_mean_distance_ratio,sample_max_distance_ratio,"
                     "sample_recall_at_k\n";
        for (const std::string& method :
             {"Median", "TopDown", "FarthestPivot", "Greedy"}) {
            benchmarkMethod(method, dataset, points, queries, truth,
                            k, seed, eps);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
