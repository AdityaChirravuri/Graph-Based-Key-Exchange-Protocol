#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <numeric>
#include <random>
#include <cstdio>  // for freopen()

const int PORT = 8080;
const int GRAPH_SIZE = 5;

// Create a random undirected graph
std::vector<std::vector<int>> createGraph() {
    std::vector<std::vector<int>> graph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE, 0));
    srand(time(0));
    for (int i = 0; i < GRAPH_SIZE; ++i) {
        for (int j = i + 1; j < GRAPH_SIZE; ++j) {
            graph[i][j] = graph[j][i] = rand() % 2;
        }
    }
    return graph;
}

// Apply permutation to graph
std::vector<std::vector<int>> permuteGraph(const std::vector<std::vector<int>>& graph, const std::vector<int>& perm) {
    std::vector<std::vector<int>> permuted(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE, 0));
    for (int i = 0; i < GRAPH_SIZE; ++i)
        for (int j = 0; j < GRAPH_SIZE; ++j)
            permuted[perm[i]][perm[j]] = graph[i][j];
    return permuted;
}

// Print 2D matrix
void printMatrix(const std::vector<std::vector<int>>& matrix, const std::string& name) {
    std::cout << "\n[" << name << "]\n";
    for (const auto& row : matrix) {
        for (int val : row) std::cout << val << " ";
        std::cout << "\n";
    }
}

void printVector(const std::vector<int>& vec, const std::string& name) {
    std::cout << "[" << name << "]: ";
    for (int val : vec) std::cout << val << " ";
    std::cout << "\n";
}

int main() {
    // Logging
    freopen("server_log.txt", "w", stdout);
    freopen("server_err.txt", "w", stderr);
    std::cout << "[SERVER] Logging started...\n";

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Socket setup
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[ERROR] Bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 3);
    std::cout << "[INFO] Server listening on port " << PORT << "\n";
    std::cout << "[INFO] Waiting for client connection...\n";

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("[ERROR] Accept failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "[INFO] Client connected\n";

    // Step 1: Create graph G
    auto originalGraph = createGraph();
    printMatrix(originalGraph, "Original Graph G");

    // Step 2: Generate P_s
    std::vector<int> perm_s(GRAPH_SIZE);
    std::iota(perm_s.begin(), perm_s.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(perm_s.begin(), perm_s.end(), g);
    printVector(perm_s, "Server Permutation P_s");

    // Step 3: G_s = P_s(G)
    auto serverGraph = permuteGraph(originalGraph, perm_s);
    printMatrix(serverGraph, "Permuted Graph G_s");

    // Step 4: Send G_s
    std::vector<int> flatGraph(GRAPH_SIZE * GRAPH_SIZE);
    for (int i = 0; i < GRAPH_SIZE; ++i)
        for (int j = 0; j < GRAPH_SIZE; ++j)
            flatGraph[i * GRAPH_SIZE + j] = serverGraph[i][j];

    send(new_socket, flatGraph.data(), flatGraph.size() * sizeof(int), 0);
    std::cout << "[INFO] Sent G_s to client\n";

    // Step 5: Receive G_sc
    std::vector<int> flatClientGraph(GRAPH_SIZE * GRAPH_SIZE);
    ssize_t bytesReceived = read(new_socket, flatClientGraph.data(), flatClientGraph.size() * sizeof(int));
    if (bytesReceived != static_cast<ssize_t>(flatClientGraph.size() * sizeof(int))) {
        std::cerr << "[ERROR] Incomplete G_sc received\n";
        close(new_socket);
        return 1;
    }

    std::vector<std::vector<int>> clientGraph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE));
    for (int i = 0; i < GRAPH_SIZE; ++i)
        for (int j = 0; j < GRAPH_SIZE; ++j)
            clientGraph[i][j] = flatClientGraph[i * GRAPH_SIZE + j];
    printMatrix(clientGraph, "Received Graph G_sc from Client");

    // Step 6: Receive P_c
    std::vector<int> perm_c(GRAPH_SIZE);
    ssize_t permBytes = read(new_socket, perm_c.data(), GRAPH_SIZE * sizeof(int));
    if (permBytes != static_cast<ssize_t>(GRAPH_SIZE * sizeof(int))) {
        std::cerr << "[ERROR] Failed to receive client permutation\n";
        close(new_socket);
        return 1;
    }
    printVector(perm_c, "Received Client Permutation P_c");

    // Step 7: Recover original G by reversing P_s
    std::vector<int> inv_perm_s(GRAPH_SIZE);
    for (int i = 0; i < GRAPH_SIZE; ++i)
        inv_perm_s[perm_s[i]] = i;
    auto recoveredGraph = permuteGraph(serverGraph, inv_perm_s);
    printMatrix(recoveredGraph, "Recovered G from G_s using P_s^-1");

    // Step 8: Apply P_c to recovered G to get expected G_sc
    auto expectedGraph = permuteGraph(recoveredGraph, perm_c);
    printMatrix(expectedGraph, "Expected G_sc (for verification)");

    // Step 9: Compare expectedGraph and clientGraph
    if (expectedGraph == clientGraph) {
        std::cout << "✅ Graphs are isomorphic — key exchange successful.\n";
    } else {
        std::cout << "❌ Graphs are NOT isomorphic — communication aborted.\n";
    }

    close(new_socket);
    close(server_fd);
    std::cout << "[INFO] Server shutdown complete\n";
    return 0;
}
