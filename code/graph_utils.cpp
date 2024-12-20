#include "graph_utils.h"
#include <set>
#include <fstream>
#include <random>
#include <cstdio>
#include <algorithm>
// #include <thread>
// #include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>

using namespace std;
using namespace Eigen;

//
class Graph {
public:
    std::unordered_map<int, std::vector<int>> adjList;
    void addEdge(int u, int v) {
        adjList[u].push_back(v);
        adjList[v].push_back(u);
    }
    void removeEdge(int node1, int node2) {
        if (adjList.find(node1) != adjList.end()) {
            auto& neighbors = adjList[node1];
            neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), node2), neighbors.end());
        }
        if (adjList.find(node2) != adjList.end()) {
            auto& neighbors = adjList[node2];
            neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), node1), neighbors.end());
        }
    }
    void removeNode(int node) {
        if (adjList.find(node) != adjList.end()) {
            for (int neighbor : adjList[node]) {
                auto& neighborList = adjList[neighbor];
                neighborList.erase(std::remove(neighborList.begin(), neighborList.end(), node), neighborList.end());
            }
            adjList.erase(node);
        }
    }
    int getNumEdges() const {
        int edgeCount = 0;
        for (const auto& pair : adjList) {
            edgeCount += pair.second.size();
        }
        return edgeCount / 2;
    }
    int getNumNodes() const {
        return adjList.size();
    }
    std::unordered_set<int> getNeighbors(int node) const {
    auto it = adjList.find(node);
    if (it != adjList.end()) {
        return std::unordered_set<int>(it->second.begin(), it->second.end());  // Convert vector to set
        }
        return {};
    }
    bool dfs(int node, std::unordered_set<int>& visited) const {
        visited.insert(node);
        for (int neighbor : adjList.at(node)) {
            if (visited.find(neighbor) == visited.end()) {
                dfs(neighbor, visited);
            }
        }
        return true;
    }
    bool isConnected() const {
        if (adjList.empty()) return true;
        std::unordered_set<int> visited;
        dfs(adjList.begin()->first, visited);
        return visited.size() == adjList.size();
    }
    void readFromFile(const std::string& filename) {
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }
        std::string line;
        int node1, node2;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            ss >> node1 >> node2;
            if (ss) {
                addEdge(node1, node2);
            }
        }
    }
    void readFromFileWithFilter(
        const std::string& filename, 
        const std::unordered_set<int>& excludedNodes
    ) {
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }
        std::string line;
        int node1, node2;
        while (std::getline(infile, line)) {
            std::stringstream ss(line);
            ss >> node1 >> node2;
            if (excludedNodes.find(node1) == excludedNodes.end() && 
                excludedNodes.find(node2) == excludedNodes.end()) {
                addEdge(node1, node2);
            }
        }
    }
    std::vector<std::pair<int, int>> getPathEdges(const std::vector<int>& path) {
        std::vector<std::pair<int, int>> edges;
        if (path.size() < 2) {
            return edges;
        }

        for (size_t i = 0; i < path.size() - 1; ++i) {
            edges.emplace_back(path[i], path[i + 1]);
        }

        return edges;
    }
    std::pair<std::vector<std::pair<int, int>>, int> randomEdgeWalk(int startNode, int targetNode, int maxLength) {
        std::vector<std::pair<int, int>> pathEdges;
        std::vector<int> path;
        std::unordered_set<int> visited;
        std::srand(std::time(nullptr));  
        std::default_random_engine rng(std::rand());
        int walkLength = 0;
        int currentNode = startNode;
        visited.insert(currentNode);
        for (int step = 0; step < maxLength; ++step) {
            if (currentNode == targetNode || adjList[currentNode].empty()) {
                break;
            }
            std::vector<int> unvisitedNeighbors;
            for (int neighbor : adjList[currentNode]) {
                if (visited.find(neighbor) == visited.end()) {
                    unvisitedNeighbors.push_back(neighbor);
                }
            }
            if (unvisitedNeighbors.empty()) {
                break;
            }
            std::uniform_int_distribution<int> dist(0, unvisitedNeighbors.size() - 1);
            currentNode = unvisitedNeighbors[dist(rng)];
            path.push_back(currentNode);
            visited.insert(currentNode);
            walkLength++;
        }
        pathEdges = getPathEdges(path);
        return {pathEdges, walkLength};
    }

    std::pair<std::vector<std::vector<std::pair<int, int>>>, std::vector<int>> collectAllPaths(int targetNode, int maxLength, int rho) {
        std::vector<std::vector<std::pair<int, int>>> pathEdgesAll;
        std::vector<int> pathLengthAll;
        
        for (int i = 0; i < rho; ++i) {
            std::vector<std::vector<std::pair<int, int>>> localPathEdges;
            std::vector<int> localPathLengths;
            for (const auto& pair : adjList) {
                int node1 = pair.first;
                for (int node2 : pair.second) {
                    if (node1 < node2 && (node1 != targetNode || node2 != targetNode)) {
                        std::pair<std::vector<std::pair<int, int>>, int> path1 = randomEdgeWalk(node1, targetNode, maxLength);
                        std::pair<std::vector<std::pair<int, int>>, int> path2 = randomEdgeWalk(node2, targetNode, maxLength);
                        if (!path1.first.empty() && !path2.first.empty()) {
                            localPathEdges.push_back(path1.first);
                            localPathLengths.push_back(path1.second);
                            localPathEdges.push_back(path2.first);
                            localPathLengths.push_back(path2.second);
                        }
                    }
                }
            }
            pathEdgesAll.insert(pathEdgesAll.end(), localPathEdges.begin(), localPathEdges.end());
            pathLengthAll.insert(pathLengthAll.end(), localPathLengths.begin(), localPathLengths.end());
        };
        return {pathEdgesAll, pathLengthAll};
    }
    
    std::pair<std::vector<std::pair<int, int>>, std::vector<double>> removeEdgeResistanceSum(Graph& g, int targetNode,
     const std::vector<std::vector<std::pair<int, int>>>& pathEdgesAll, const std::vector<int>& pathLengthAll, int rho) {
        std::vector<std::pair<int, int>> removeEdge;
        std::vector<double> removeEdgeResistance;
        std::vector<std::pair<int, int>> edgesToRemove;
        for (const auto& pair : g.adjList) {
            int node1 = pair.first;
            for (int node2: pair.second) {
                double weights_u = 0;
                double weights_xy = 0;
                for (size_t i = 0; i < pathEdgesAll.size(); i+=2) {
                    for (const auto& edge : pathEdgesAll[i]){
                        if (pathEdgesAll[i].back().second == targetNode || pathEdgesAll[i].back().first == targetNode) {    
                            if ((edge.first == node1 && edge.second == node2) || (edge.first == node1 && edge.second == node2)) {
                                weights_u += (abs(pathLengthAll[i] * (pathLengthAll[i+1]))) / (pathLengthAll[i] + pathLengthAll[ i+1]) ;
                                weights_xy += (abs(pathLengthAll[i] - pathLengthAll[i+1]));
                            }
                        }
                    }
                }
                removeEdge.emplace_back(node1, node2);
                if (weights_xy == 0) {
                    removeEdgeResistance.push_back(0);
                } else {
                    removeEdgeResistance.push_back(weights_u);
                }
            }
        }

        return {removeEdge, removeEdgeResistance};
    }
};

class CSR {
private:
    std::shared_ptr<std::vector<int>> row_ptr = std::make_shared<std::vector<int>>();
    std::shared_ptr<std::vector<int>> col_idx = std::make_shared<std::vector<int>>();
    std::shared_ptr<std::vector<int>> dirct_vertex = std::make_shared<std::vector<int>>();
    std::shared_ptr<std::vector<int>> dirct_edge = std::make_shared<std::vector<int>>();
    int num_nodes;             // Number of nodes in the graph
    int num_edges;             // Number of edges in the graph
    // std::mutex edge_scores_mutex;

public:
    CSR() : num_nodes(0), num_edges(0) {}

    void readFromFileWithFilter(const std::string& filename, const std::unordered_set<int>& excludedNodes) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Unable to open file " << filename << std::endl;
            return;
        }
        std::vector<std::pair<int, int>> edges;
        int max_node = -1;
        int u, v;
        while (file >> u >> v) {
            if (u <= 0 || v <= 0) {
                std::cerr << "Invalid node indices in the file!" << std::endl;
                return;
            }
            u -= 1;
            v -= 1;
            if (excludedNodes.find(u) == excludedNodes.end() && excludedNodes.find(v) == excludedNodes.end()) {
                edges.emplace_back(u, v);
                max_node = std::max(max_node, std::max(u, v));
            }
        }
        file.close();
        row_ptr->resize(max_node + 2, 0);
        std::vector<int> node_degree(max_node + 1, 0);
        for (const auto& edge : edges) {
            node_degree[edge.first]++;
            node_degree[edge.second]++;
        }
        for (int i = 0; i < node_degree.size(); ++i) {
            (*row_ptr)[i + 1] = (*row_ptr)[i] + node_degree[i];
        }
        col_idx->resize(edges.size() * 2);
        std::vector<int> current_row_count(max_node + 1, 0);
        for (const auto& edge : edges) {
            int row = edge.first;
            int dest = edge.second;
            int index_1 = (*row_ptr)[row] + current_row_count[row];
            int index_2 = (*row_ptr)[dest] + current_row_count[dest];
            (*col_idx)[index_1] = dest;
            (*col_idx)[index_2] = row;
            current_row_count[row]++;
            current_row_count[dest]++;
        }
        dirct_vertex->resize(max_node + 2, 0);
        dirct_edge->resize(edges.size());
        num_nodes = max_node + 1;
        num_edges = edges.size();
        for (const auto& edge : edges) {
            (*dirct_vertex)[edge.first + 1]++;
        }
        for (int i = 1; i < dirct_vertex->size(); ++i) {
            (*dirct_vertex)[i] += (*dirct_vertex)[i - 1];
        }
        std::vector<int> temp_position = (*dirct_vertex);
        for (const auto& edge : edges) {
            int u = edge.first;
            int v = edge.second;
            (*dirct_edge)[temp_position[u]++] = v;
        }
    }
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Unable to open file " << filename << std::endl;
            return;
        }
        std::vector<std::pair<int, int>> edges;
        int max_node = -1;
        int u, v;
        while (file >> u >> v) {
            if (u <= 0 || v <= 0) {
                std::cerr << "Invalid node indices in the file!" << std::endl;
                return;
            }
            u -= 1;
            v -= 1;
            edges.emplace_back(u, v);
            max_node = std::max(max_node, std::max(u, v));
        }
        file.close();
        row_ptr->resize(max_node + 2, 0);
        std::vector<int> node_degree(max_node + 1, 0);
        for (const auto& edge : edges) {
            node_degree[edge.first]++;
            node_degree[edge.second]++;
        }
        for (int i = 0; i < node_degree.size(); ++i) {
            (*row_ptr)[i + 1] = (*row_ptr)[i] + node_degree[i];
        }
        // Fill column indices
        col_idx->resize(edges.size() * 2);
        std::vector<int> current_row_count(max_node + 1, 0);
        for (const auto& edge : edges) {
            int row = edge.first;
            int dest = edge.second;
            int index_1 = (*row_ptr)[row] + current_row_count[row];
            int index_2 = (*row_ptr)[dest] + current_row_count[dest];
            (*col_idx)[index_1] = dest;
            (*col_idx)[index_2] = row;
            current_row_count[row]++;
            current_row_count[dest]++;
        }

        // Store direct vertex and edges for the original graph
        dirct_vertex->resize(max_node + 2, 0);
        dirct_edge->resize(edges.size());
        num_nodes = max_node + 1;
        num_edges = edges.size();
        // current_row_count.assign(max_node + 1, 0);
        for (const auto& edge : edges) {
            (*dirct_vertex)[edge.first + 1]++;
        }
        for (int i = 1; i < dirct_vertex->size(); ++i) {
            (*dirct_vertex)[i] += (*dirct_vertex)[i - 1];
        }
        std::vector<int> temp_position = (*dirct_vertex);
        for (const auto& edge : edges) {
            int u = edge.first;
            int v = edge.second;
            (*dirct_edge)[temp_position[u]++] = v;
        }
    }
    void addEdge(int u, int v) {
        throw std::runtime_error("Dynamic edge addition not supported in CSR.");
    }
    std::vector<int> getNeighbors(int node) const {
        if (node < 0 || node >= num_nodes) {
            return {};
        }
        int start = (*dirct_vertex)[node];
        int end = (*dirct_vertex)[node + 1];
        return std::vector<int>(dirct_edge->begin() + start, dirct_edge->begin() + end);
    }
    void printCSR() const {
        std::cout << "row_ptr: ";
        for (const auto& val : *row_ptr) {
            std::cout << val << " ";
        }
        std::cout << "\ncol_idx: ";
        for (const auto& val : *col_idx) {
            std::cout << val << " ";
        }
        std::cout << "\ndirct_vertex: ";
        for (const auto& val : *dirct_vertex) {
            std::cout << val << " ";
        }
        std::cout << "\ndirct_edge: ";
        for (const auto& val : *dirct_edge) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    int getNumNodes() const {
        return num_nodes;
    }

    int getNumEdges() const {
        return num_edges;
    }
    const std::vector<int>& getRowPtr() const {
        return *row_ptr;
    }
    const std::vector<int>& getColIdx() const {
        return *col_idx;
    }
    float randomWalk(int source, int target, int steps) const {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0, 1.0);
        int current_node = source;
        for (int current_step = 0; current_step < steps; ++current_step) {
            int start = (*row_ptr)[current_node];
            int end = (*row_ptr)[current_node + 1];
            int degree = end - start;
            if (degree == 0) {
                return 0.0f;
            }
            int next_index = start + (std::rand() % degree);
            current_node = (*col_idx)[next_index];;
            if (current_node == target) {
                return 1.0f / static_cast<float>(current_step);
            }
        }
        return 0.0f;
    }

    std::vector<float> calculateEdgeScore(int target, int rho, int max_length) {
        auto edge_scores = std::make_shared<std::vector<float>>(dirct_edge->size(), 0);
        int target_index = target;
        // std::vector<std::thread> threads;
        // threads.reserve(num_nodes);
        float max_score = 1e9;
        for (int i = 0; i < num_nodes; ++i) 
            // threads.emplace_back([&, i]() {
            for (int j = (*dirct_vertex)[i] ; j < (*dirct_vertex)[i + 1]; ++j) {
                int u = i;
                int v = (*dirct_edge)[j];
                float value = 0;
                for (int count = 0; count < rho; ++count) {
                    float u_score = randomWalk(u, target_index, max_length);
                    float v_score = randomWalk(v, target_index, max_length);
                    u_score = std::min(u_score, max_score);
                    v_score = std::min(v_score, max_score);
                    if (u_score > 0 && v_score > 0) {
                            value += u_score + v_score;
                        }
                }
                // std::lock_guard<std::mutex> lock(edge_scores_mutex);
                (*edge_scores)[j] += value / static_cast<float>(rho);
                }
            // });
            // threads.emplace_back([&, i]() {
            //     int start = (*dirct_vertex)[i];
            //     int end = (*dirct_vertex)[i + 1];
            //     for (int j = start; j < end; ++j) {
            //         int neighbor = (*dirct_edge)[j];
            //         float score_sum = 0;
            //         for (int count = 0; count < rho; ++count) {
            //             float v1_score = randomWalk(i, target_index, max_length);
            //             float v2_score = randomWalk(neighbor, target_index, max_length);
            //             if (v1_score != 0 && v2_score != 0) {
            //                 score_sum += v1_score + v2_score;
            //             }
            //         }
            //         std::lock_guard<std::mutex> lock(edge_scores_mutex);
            //         (*edge_scores)[j] = score_sum / static_cast<float>(rho);
            //     }
            // });
        // }
        // for (auto& thread : threads) {
        //     if (thread.joinable()) {
        //         thread.join();
        //     }
        // }
        return *edge_scores;
    }
};
// Function to read the graph, calculate edge scores, and return vector<Edge>
EdgeList APPROXISC(string filename, int k, int target, int maxLength, double epsilon) {
    CSR g;
    g.loadFromFile(filename);
    // g.printCSR();
    int rho = static_cast<int>(0.1 * log2(g.getNumNodes()) / pow(epsilon, 2) / maxLength);
    std::vector<float> edge_scores;
    std::vector<std::tuple<int, int, float>> edges_with_scores;
    const auto& row_ptr = g.getRowPtr();
    const auto& col_idx = g.getColIdx();
    edge_scores = g.calculateEdgeScore(target, rho, maxLength * 0.5);
    for (int i = 0; i < g.getNumEdges(); ++i) {
        int source_vertex = -1;
        for (int j = 0; j < g.getNumNodes(); ++j) {
            if (i >= row_ptr[j] && i < row_ptr[j + 1]) {
                source_vertex = j;
                break;
            }
        }
        int target_vertex = col_idx[i];
        edges_with_scores.emplace_back(source_vertex, target_vertex, edge_scores[i]);
    }
    std::sort(edges_with_scores.begin(), edges_with_scores.end(), [](const std::tuple<int, int, float>& a, const std::tuple<int, int, float>& b) {
        return std::get<2>(a) > std::get<2>(b);
    });
    EdgeList P;
    for (int i = 0; i < k && i < edges_with_scores.size(); ++i) {
        int source_vertex = std::get<0>(edges_with_scores[i]);
        int target_vertex = std::get<1>(edges_with_scores[i]);
        P.push_back({source_vertex, target_vertex});
    }
    return P;
}

SparseMatrix<double> createLaplacianMatrix(int num_nodes, const EdgeList& edges) {
    SparseMatrix<double> L(num_nodes, num_nodes);
    VectorXd degrees = VectorXd::Zero(num_nodes);
    for (const auto& edge : edges) {
        int u = edge.first;
        int v = edge.second;
        degrees[u]++;
        degrees[v]++;
        L.coeffRef(u, v) -= 1;
        L.coeffRef(v, u) -= 1;
    }
    for (int i = 0; i < num_nodes; ++i) {
        L.coeffRef(i, i) = degrees[i];
    }
    return L;
}


MatrixXd computePseudoinverse(const SparseMatrix<double>& L) {
    MatrixXd L_dense = MatrixXd(L);
    JacobiSVD<MatrixXd> svd(L_dense, ComputeFullU | ComputeFullV);
    const double tolerance = 1e-5;
    VectorXd singularValuesInv = svd.singularValues();
    for (int i = 0; i < singularValuesInv.size(); ++i) {
        singularValuesInv(i) = (singularValuesInv(i) > tolerance) ? 1.0 / singularValuesInv(i) : 0;
    }
    return svd.matrixV() * singularValuesInv.asDiagonal() * svd.matrixU().transpose();
}

vector<int> selectRandomNodes(int max_node, int count) {
    set<int> selected_nodes;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, max_node - 1);
    while (selected_nodes.size() < count) {
        selected_nodes.insert(dis(gen));
    }
    return vector<int>(selected_nodes.begin(), selected_nodes.end());
}

vector<Edge> removeSpecificEdges(vector<Edge>& edges, const vector<Edge>& edges_to_remove) {
    vector<Edge> removed_edges;
    for (const Edge& remove_edge : edges_to_remove) {
        auto it = find(edges.begin(), edges.end(), remove_edge);
        if (it != edges.end()) {
            removed_edges.push_back(*it);
            edges.erase(it);
        }
    }
    return removed_edges;
}

bool isConnected(int n, EdgeList& edges) {
    vector<vector<int>> adjList(n);
    for (auto& e : edges) {
        adjList[e.first].push_back(e.second);
        adjList[e.second].push_back(e.first);
    }
    vector<bool> visited(n, false);
    queue<int> q;
    q.push(0);
    visited[0] = true;
    while (!q.empty()) {
        int node = q.front();
        q.pop();
        for (int neighbor : adjList[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                q.push(neighbor);
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) return false;
    }
    return true;
}

EdgeList OPTIMUM(string filename, int target, int k) {
    EdgeList P;
    ifstream infile(filename);
    if (!infile) {
        cerr << "Error: Could not open file " << filename << endl;
        return P;
    }
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        cerr << "Error: Could not open file " << filename << endl;
        return P;
    }
    vector<Edge> edges;
    int n = 0;
    int u, v;
    while (fscanf(file, "%d %d", &u, &v) == 2) {
        edges.emplace_back(u - 1, v - 1);
        n = max(n, max(u, v));
    }
    fclose(file);
    SparseMatrix<double> L = createLaplacianMatrix(n, edges);
    MatrixXd L_dagger = computePseudoinverse(L);
    for (int i = 0; i < k; i++) {
        vector<pair<double, Edge>> information_centrality_list;
        for (auto& e : edges) {
            int x = e.first, y = e.second;
            EdgeList edges_copy = edges;
            edges_copy.erase(remove(edges_copy.begin(), edges_copy.end(), e), edges_copy.end());

            SparseMatrix<double> L_copy = createLaplacianMatrix(n, edges_copy);
            MatrixXd L_dagger_copy = computePseudoinverse(L_copy);
            
            double information_centrality_sum = 0.0;
            double effective_resistance_of_node = n * L_dagger_copy(target, target) + L_dagger_copy.trace();
            double information_centrality = n / effective_resistance_of_node;
            information_centrality_sum += information_centrality;

            information_centrality_list.push_back(make_pair(information_centrality_sum / k, e));
        }

        auto arg_min = *min_element(information_centrality_list.begin(), information_centrality_list.end(),
                                    [](const pair<double, Edge>& a, const pair<double, Edge>& b) {
                                        return a.first < b.first;
                                    });
        P.push_back(arg_min.second);
        edges.erase(remove(edges.begin(), edges.end(), arg_min.second), edges.end());
    }
    return P;
}

EdgeList EXACTSM(string filename, int target, int k) {
    ifstream infile(filename);
    EdgeList P;
    if (!infile) {
        cerr << "Error: Could not open file " << filename << endl;
        return P;
    }
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        cerr << "Error: Could not open file " << filename << endl;
        return P;
    }
    vector<Edge> edges;
    int n = 0;
    int u, v;
    while (fscanf(file, "%d %d", &u, &v) == 2) {
        edges.emplace_back(u - 1, v - 1);
        n = max(n, max(u, v));
    }
    fclose(file);
    SparseMatrix<double> L = createLaplacianMatrix(n, edges);
    MatrixXd L_dagger = computePseudoinverse(L);
    for (int i = 0; i < k; ++i) {
        vector<pair<double, Edge>> information_centrality_list;
        for (auto& e : edges) {
            double information_centrality = 0;
            int x = e.first, y = e.second;
            vector<Edge> edges_copy = edges;
            edges_copy.erase(remove(edges_copy.begin(), edges_copy.end(), e), edges_copy.end());
            if (isConnected(n, edges_copy)) {
                VectorXd b_e = VectorXd::Zero(n);
                b_e(x) = 1;
                b_e(y) = -1;
                double a = 1 - b_e.transpose() * L_dagger * b_e;
                double b = b_e.transpose() * L_dagger * L_dagger * b_e;
                VectorXd L_dagger_b_e = L_dagger * b_e;
                double c = L_dagger_b_e(target) * L_dagger_b_e(target);
                double numerator = -(n * b + n * n * c);
                double term1 = n * a * L_dagger(target, target) + n * c;
                double term2 = a * L_dagger.trace() + b;
                double term3 = n * L_dagger(target, target) + L_dagger.trace();
                double denominator = (term1 + term2) * term3;
                information_centrality += numerator / denominator;
            }
            information_centrality_list.push_back(make_pair(information_centrality, e));
        }
        auto arg_min = *min_element(information_centrality_list.begin(), information_centrality_list.end(),
                                    [](const pair<double, Edge>& a, const pair<double, Edge>& b) {
                                        return a.first < b.first;
                                    });
        P.push_back(arg_min.second);
        edges.erase(remove(edges.begin(), edges.end(), arg_min.second), edges.end());
        VectorXd b_ei = VectorXd::Zero(n);
        b_ei(arg_min.second.first) = 1;
        b_ei(arg_min.second.second) = -1;
        L_dagger += (L_dagger * b_ei * b_ei.transpose() * L_dagger) / (1 - b_ei.transpose() * L_dagger * b_ei);
    }
    return P;
}

std::string getOutputFilename(const std::string& inputFilename, const std::string& algorithmName) {
    size_t lastSlash = inputFilename.find_last_of("/\\");
    size_t lastDot = inputFilename.find_last_of('.');
    std::string baseName = inputFilename.substr(
        lastSlash + 1, 
        lastDot - lastSlash - 1
    );
    return "result/" + baseName + algorithmName + "_result.txt";
}

void printTargetNodesAndEdges(const std::string& inputFilename, const std::string& algorithmName, const std::vector<int>& T, const std::vector<std::pair<int, std::vector<Edge>>>& all_P) {
    std::string outputFilename = getOutputFilename(inputFilename, algorithmName);
    std::ifstream existingFile(outputFilename);
    if (existingFile.good()) {
        existingFile.close();
        if (std::remove(outputFilename.c_str()) == 0) {
            std::cout << "File " << outputFilename << " already exists and will be replaced.\n";
        } else {
            std::cerr << "Error deleting existing file " << outputFilename << ".\n";
            return;
        }
    }
    std::ofstream outFile(outputFilename);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file " << outputFilename << " for writing." << std::endl;
        return;
    }
    outFile << "Target nodes: [";
    for (size_t i = 0; i < T.size(); ++i) {
        outFile << T[i];
        if (i < T.size() - 1) {
            outFile << ", ";
        }
    }
    outFile << "]\n\n";
    outFile << "Edges to remove per target node:\n[\n";
    for (size_t i = 0; i < all_P.size(); ++i) {
        const auto& entry = all_P[i];
        const std::vector<Edge>& P_edges = entry.second;
        outFile << "  [";
        for (size_t j = 0; j < P_edges.size(); ++j) {
            outFile << "(" << P_edges[j].first << ", " << P_edges[j].second << ")";
            if (j < P_edges.size() - 1) {
                outFile << ", ";
            }
        }
        outFile << "]";
        if (i < all_P.size() - 1) {
            outFile << ",\n";
        } else {
            outFile << "\n";
        }
    }
    outFile << "]\n";
    outFile.close();
    std::cout << "Data saved to " << outputFilename << "\n";
}

int max_random_walk_length(string filename, int target, double gamma) {
    Graph g;
    g.readFromFile(filename);
    int n = g.getNumNodes();
    int m = g.getNumEdges();
    std::unordered_map<int, int> degrees;
    for (const auto& node : g.adjList) {
        degrees[node.first] = node.second.size();
    }
    degrees.erase(target);
    double d_norm = 0.0;
    for (const auto& entry : degrees) {
        d_norm += std::pow(entry.second, 2);
    }
    d_norm = std::sqrt(d_norm);
    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(n, n);
    for (const auto& node : g.adjList) {
        int u = node.first;
        for (int v : node.second) {
            A(u - 1, v - 1) = 1.0;
            A(v - 1, u - 1) = 1.0;
        }
    }
    Eigen::MatrixXd A_reduced = Eigen::MatrixXd::Zero(n - 1, n - 1);
    std::unordered_map<int, int> nodeMapping;
    int newIndex = 0;
    for (int u = 0; u < n; ++u) {
        if (u == target) continue;
        nodeMapping[u] = newIndex;
        newIndex++;
        for (int v : g.getNeighbors(u)) {
            if (v == target) continue;
            A_reduced(nodeMapping[u], nodeMapping[v]) = 1.0;
            A_reduced(nodeMapping[v], nodeMapping[u]) = 1.0;
        }
    }
    Eigen::VectorXcd eigenvalues = A_reduced.eigenvalues();
    double spectral_radius = eigenvalues.cwiseAbs().maxCoeff();
    if (spectral_radius <= 1) {
        std::cerr << "Error: Spectral radius is less than or equal to 1, invalid for max_length calculation!" << std::endl;
        return -1;
    }
    int max_length = static_cast<int>(0.5 * (std::log(m * gamma / std::sqrt(n - 1)) * d_norm) / std::log(spectral_radius));
    return max_length;
}

EdgeList APPROXISC_fail(string filename, int numberOfEdge, int targetNode, int maxLength, double epsilon) {
    Graph g;
    g.readFromFile(filename);
    int rho = static_cast<int>(log(g.getNumNodes()) / pow(epsilon, 2) / 200);
    std::vector<std::pair<int, int>> P;
    int n = g.getNumNodes();
    std::unordered_set<int> excludedNodes;
    n = g.getNumNodes();
    for (int i = 0; i < numberOfEdge; ++i) {
        auto allPaths = g.collectAllPaths(targetNode, maxLength, rho);
        const auto& pathEdgesAll = allPaths.first;
        const auto& pathLengthAll = allPaths.second;
        auto allResistance = g.removeEdgeResistanceSum(g, targetNode, pathEdgesAll, pathLengthAll, rho);
        const auto& pathEdges = allResistance.first;
        const auto& pathEdgesResistance = allResistance.second;
        std::pair<int, int> maxEdge;
        double maxResistance = std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < pathEdges.size(); ++i) {
            if (pathEdgesResistance[i] < maxResistance) {
                maxResistance = pathEdgesResistance[i];
                maxEdge = pathEdges[i];
            }
        }
        g.removeEdge(maxEdge.first, maxEdge.second);
        P.push_back({maxEdge.first - 1, maxEdge.second - 1});
    }
    return P;
}

void optimizedRandomNodeSelection(int n, int t, int targetNode, std::unordered_set<int>& excludedNodes) {
    std::vector<int> nodes;
    for (int i = 0; i < n; ++i) {
        if (i != targetNode) {
            nodes.push_back(i);
        }
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(nodes.begin(), nodes.end(), gen);
    excludedNodes.insert(nodes.begin(), nodes.begin() + t);
}

EdgeList FASTICM(std::string filename, int k, int targetNode, 
    int maxLength, double alpha, int phi) {
    CSR p;
    p.loadFromFile(filename);
    // g.printCSR();
    int n = p.getNumNodes();
    double beta = alpha / 2;
    double epsilon = (alpha / 2) / phi;
    int t = static_cast<int>(0.5 * phi * sqrt(n * log(n)) / beta);
    t = (t > n) ? 0 : n - t;
    std::unordered_set<int> excludedNodes;
    optimizedRandomNodeSelection(n, t, targetNode, excludedNodes);
    CSR g;
    g.readFromFileWithFilter(filename, excludedNodes);
    int gn = g.getNumNodes();
    int rho = static_cast<int>(0.01 * log2(gn) / pow(epsilon, 2) / maxLength);
    std::vector<float> edge_scores;
    std::vector<std::tuple<int, int, float>> edges_with_scores;
    const auto& row_ptr = g.getRowPtr();
    const auto& col_idx = g.getColIdx();
    edge_scores = g.calculateEdgeScore(targetNode, rho, maxLength * 0.5);
    for (int i = 0; i < g.getNumEdges(); ++i) {
        int source_vertex = -1;
        for (int j = 0; j < g.getNumNodes(); ++j) {
            if (i >= row_ptr[j] && i < row_ptr[j + 1]) {
                source_vertex = j;
                break;
            }
        }
        int target_vertex = col_idx[i];
        edges_with_scores.emplace_back(source_vertex, target_vertex, edge_scores[i]);
    }
    std::sort(edges_with_scores.begin(), edges_with_scores.end(), [](const std::tuple<int, int, float>& a, const std::tuple<int, int, float>& b) {
        return std::get<2>(a) > std::get<2>(b);
    });
    EdgeList P;
    for (int i = 0; i < k && i < edges_with_scores.size(); ++i) {
        int source_vertex = std::get<0>(edges_with_scores[i]);
        int target_vertex = std::get<1>(edges_with_scores[i]);
        P.push_back({source_vertex, target_vertex});
    }
    return P;
}
EdgeList FASTICM_fail(std::string filename, int numberOfEdge, int targetNode, 
    int maxLength, double alpha, int phi) {
    Graph p;
    p.readFromFile(filename);
    int n = p.getNumNodes();
    double beta = alpha / 2;
    double epsilon = (alpha / 2) / phi;
    int t = static_cast<int>(0.5 * phi * sqrt(n * log(n)) / beta);
    int rho = static_cast<int>(log2(n) / (pow(epsilon, 2) * maxLength));
    t = (t > n) ? 0 : n - t;
    std::unordered_set<int> excludedNodes;
    optimizedRandomNodeSelection(n, t, targetNode, excludedNodes);
    Graph g;
    g.readFromFileWithFilter(filename, excludedNodes);
    n = g.getNumNodes();
    std::vector<std::pair<int, int>> P;
    auto allPaths = g.collectAllPaths(targetNode, maxLength, rho);
    for (int i = 0; i < numberOfEdge; ++i) {
        const auto& pathEdgesAll = allPaths.first;
        const auto& pathLengthAll = allPaths.second;
        auto allResistance = g.removeEdgeResistanceSum(g, targetNode, pathEdgesAll, pathLengthAll, rho);
        const auto& pathEdges = allResistance.first;
        const auto& pathEdgesResistance = allResistance.second;
        std::pair<int, int> maxEdge;
        double maxResistance = std::numeric_limits<double>::infinity();
        for (size_t i = 10; i < pathEdges.size(); ++i) {
            if (pathEdgesResistance[i] < maxResistance) {
                maxResistance = pathEdgesResistance[i];
                maxEdge = pathEdges[i];
            }
        }
        g.removeEdge(maxEdge.first, maxEdge.second);
        P.push_back({maxEdge.first - 1, maxEdge.second - 1});
    }
    return P;
}