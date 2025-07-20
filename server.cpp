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

// Function to check if two graphs are isomorphic
bool areIsomorphic(const std::vector<std::vector<int>>& graph1, const std::vector<std::vector<int>>& graph2) {
    if (graph1.size() != graph2.size())
        return false;

    int n = graph1.size();
    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);

    // Try all permutations of vertices
    do {
        std::vector<std::vector<int>> permutedGraph = permuteGraph(graph1, perm);
        if (permutedGraph == graph2) {
            return true; // Found an isomorphism
        }
    } while (std::next_permutation(perm.begin(), perm.end()));

    return false; // No isomorphism found
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Socket setup
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    // Wait for client connection
    std::cout << "Waiting for client connection...\n";
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Connection accept error");
        exit(EXIT_FAILURE);
    }

    // Create a public graph and permute it
    auto publicGraph = createGraph();
    std::vector<int> perm(GRAPH_SIZE);
    std::iota(perm.begin(), perm.end(), 0);

    // Initialize random generator for shuffling
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(perm.begin(), perm.end(), g);

    // Permute the graph
    auto permutedGraph = permuteGraph(publicGraph, perm);

    // Send permuted graph to client
    send(new_socket, permutedGraph.data(), GRAPH_SIZE * GRAPH_SIZE * sizeof(int), 0);
    std::cout << "Sent permuted graph to client\n";

    // Receive permuted graph from client
    std::vector<std::vector<int>> clientGraph(GRAPH_SIZE, std::vector<int>(GRAPH_SIZE, 0));
    read(new_socket, clientGraph.data(), GRAPH_SIZE * GRAPH_SIZE * sizeof(int));

    // Check for isomorphism
    if (areIsomorphic(permutedGraph, clientGraph)) {
        std::cout << "The graphs are isomorphic.\n";
        // Data transfer or other operations can happen here
    } else {
        std::cout << "The graphs are not isomorphic. Data transfer aborted.\n";
        close(new_socket);
        close(server_fd);
        return 0; // Exit if graphs are not isomorphic
    }

    // Close the connection
    close(new_socket);
    close(server_fd);
    return 0;
}
