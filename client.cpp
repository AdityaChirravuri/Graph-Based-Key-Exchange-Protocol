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

// Define constants
const int PORT = 8080;
const int GRAPH_SIZE = 5;

// Function to create a random graph
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

// Function to permute a graph with a given permutation vector
std::vector<std::vector<int>> permuteGraph(const std::vector<std::vector<int>>& graph, const std::vector<int>& perm) {
    std::vector<std::vector<int>> permutedGraph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE, 0));
    for (int i = 0; i < GRAPH_SIZE; ++i) {
        for (int j = 0; j < GRAPH_SIZE; ++j) {
            permutedGraph[perm[i]][perm[j]] = graph[i][j];
        }
    }
    return permutedGraph;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Receive permuted graph from server
    std::vector<std::vector<int>> serverGraph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE, 0));
    read(sock, serverGraph.data(), GRAPH_SIZE * GRAPH_SIZE * sizeof(int));

    std::cout << "Received permuted graph from server:\n";
    for (const auto& row : serverGraph) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    // Create own random graph and permute it
    auto publicGraph = createGraph();
    std::vector<int> perm(GRAPH_SIZE);
    std::iota(perm.begin(), perm.end(), 0);

    // Initialize random generator for shuffling
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(perm.begin(), perm.end(), g);

    auto permutedGraph = permuteGraph(publicGraph, perm);

    // Send permuted graph to server
    send(sock, permutedGraph.data(), GRAPH_SIZE * GRAPH_SIZE * sizeof(int), 0);
    std::cout << "Sent permuted graph to server\n";

    close(sock);
    return 0;
}
