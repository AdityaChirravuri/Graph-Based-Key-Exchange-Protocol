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

// Function to permute a graph using a given permutation
std::vector<std::vector<int>> permuteGraph(const std::vector<std::vector<int>>& graph, const std::vector<int>& perm) {
    std::vector<std::vector<int>> permutedGraph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE, 0));
    for (int i = 0; i < GRAPH_SIZE; ++i) {
        for (int j = 0; j < GRAPH_SIZE; ++j) {
            permutedGraph[perm[i]][perm[j]] = graph[i][j];
        }
    }
    return permutedGraph;
}

void printMatrix(const std::vector<std::vector<int>>& matrix, const std::string& name) {
    std::cout << "\n[" << name << "]\n";
    for (const auto& row : matrix) {
        for (int val : row)
            std::cout << val << " ";
        std::cout << "\n";
    }
}

void printVector(const std::vector<int>& vec, const std::string& name) {
    std::cout << "\n[" << name << "]: ";
    for (int v : vec)
        std::cout << v << " ";
    std::cout << "\n";
}

int main() {
    // Redirect logs to files
    freopen("client_log.txt", "w", stdout);   // Logs (std::cout)
    freopen("client_err.txt", "w", stderr);   // Errors (std::cerr)

    std::cout << "[CLIENT] Logging started..." << std::endl;

    int sock = 0;
    struct sockaddr_in serv_addr;

    // Socket creation
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[ERROR] Socket creation failed");
        return -1;
    }
    std::cout << "[INFO] Socket created\n";

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("[ERROR] Invalid address");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[ERROR] Connection Failed");
        return -1;
    }
    std::cout << "[INFO] Connected to server\n";

    // Step 1: Receive graph from server
    std::vector<int> flatGraph(GRAPH_SIZE * GRAPH_SIZE, 0);
    ssize_t bytesReceived = read(sock, flatGraph.data(), flatGraph.size() * sizeof(int));
    std::cout << "[INFO] Expecting " << flatGraph.size() * sizeof(int) << " bytes\n";

    if (bytesReceived != static_cast<ssize_t>(flatGraph.size() * sizeof(int))) {
        std::cerr << "[ERROR] Received " << bytesReceived << " bytes. Incomplete graph.\n";
        close(sock);
        return -1;
    }

    std::cout << "[INFO] Graph received from server (" << bytesReceived << " bytes)\n";

    // Convert to 2D matrix
    std::vector<std::vector<int>> serverGraph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE));
    for (int i = 0; i < GRAPH_SIZE; ++i)
        for (int j = 0; j < GRAPH_SIZE; ++j)
            serverGraph[i][j] = flatGraph[i * GRAPH_SIZE + j];

    printMatrix(serverGraph, "G_s from Server");

    // Step 2: Generate permutation P_c
    std::vector<int> perm_c(GRAPH_SIZE);
    std::iota(perm_c.begin(), perm_c.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(perm_c.begin(), perm_c.end(), g);

    printVector(perm_c, "Client Permutation P_c");

    // Step 3: Apply permutation to G_s
    auto clientGraph = permuteGraph(serverGraph, perm_c);
    printMatrix(clientGraph, "G_sc (G_s after applying P_c)");

    // Step 4: Flatten G_sc
    std::vector<int> flatClientGraph(GRAPH_SIZE * GRAPH_SIZE);
    for (int i = 0; i < GRAPH_SIZE; ++i)
        for (int j = 0; j < GRAPH_SIZE; ++j)
            flatClientGraph[i * GRAPH_SIZE + j] = clientGraph[i][j];

    // Step 5: Send G_sc
    ssize_t sentGraph = send(sock, flatClientGraph.data(), flatClientGraph.size() * sizeof(int), 0);
    std::cout << "[INFO] Sent " << sentGraph << " bytes (G_sc)\n";
    if (sentGraph != static_cast<ssize_t>(flatClientGraph.size() * sizeof(int))) {
        std::cerr << "[ERROR] Failed to send complete G_sc\n";
        close(sock);
        return -1;
    }

    // Step 6: Send P_c
    ssize_t sentPerm = send(sock, perm_c.data(), GRAPH_SIZE * sizeof(int), 0);
    std::cout << "[INFO] Sent " << sentPerm << " bytes (P_c)\n";
    if (sentPerm != static_cast<ssize_t>(GRAPH_SIZE * sizeof(int))) {
        std::cerr << "[ERROR] Failed to send permutation vector\n";
        close(sock);
        return -1;
    }

    std::cout << "[CLIENT] Graph and permutation sent. Closing connection.\n";

    close(sock);
    return 0;
}
