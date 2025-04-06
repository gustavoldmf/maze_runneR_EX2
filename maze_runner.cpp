#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <thread>
#include <mutex>
#include <chrono>
#include <memory>
#include <atomic>

// Representação do labirinto
using Maze = std::vector<std::vector<char>>;

// Estrutura para representar uma posição no labirinto
struct Position {
    int row;
    int col;
};

// Variáveis globais
std::mutex maze_mutex;
std::mutex cout_mutex;
Maze maze;
int num_rows;
int num_cols;
std::atomic<bool> exit_found{false};

// Função para carregar o labirinto de um arquivo
Position load_maze(const std::string& file_name) {
    std::ifstream ifs(file_name);
    if (!ifs) {
        std::cerr << "Erro ao abrir o arquivo: " << file_name << std::endl;
        return {-1, -1};
    }

    ifs >> num_rows >> num_cols;
    ifs.ignore();

    maze.resize(num_rows, std::vector<char>(num_cols));
    Position initial = {-1, -1};

    for (int i = 0; i < num_rows; i++) {
        std::string line;
        std::getline(ifs, line);
        for (int j = 0; j < num_cols; j++) {
            maze[i][j] = line[j];
            if (maze[i][j] == 'e') {
                initial = {i, j};
            }
        }
    }

    ifs.close();
    return initial;
}

// Função para imprimir o labirinto
void print_maze() {
    std::lock_guard<std::mutex> lock(cout_mutex);
    for(int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_cols; j++) {
            std::cout << maze[i][j];
        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

// Função para verificar se uma posição é válida
bool is_valid_position(int row, int col) {
    return (row >= 0 && row < num_rows && col >= 0 && col < num_cols && 
           (maze[row][col] == 'x' || maze[row][col] == 's'));
}

// Função principal para navegar pelo labirinto
void walk(Position pos) {
    // Se já encontrou a saída, retorna
    if (exit_found) return;

    // Verifica se encontrou a saída
    {
        std::lock_guard<std::mutex> lock(maze_mutex);
        if (maze[pos.row][pos.col] == 's') {
            exit_found = true;
            return;
        }
    }

    // Marca a posição atual como visitada
    {
        std::lock_guard<std::mutex> lock(maze_mutex);
        maze[pos.row][pos.col] = '.'; 
    }
    
    print_maze();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verifica posições adjacentes
    std::vector<Position> valid_positions;
    if (is_valid_position(pos.row, pos.col + 1)) 
        valid_positions.push_back({pos.row, pos.col + 1});
    if (is_valid_position(pos.row, pos.col - 1)) 
        valid_positions.push_back({pos.row, pos.col - 1});
    if (is_valid_position(pos.row - 1, pos.col)) 
        valid_positions.push_back({pos.row - 1, pos.col});
    if (is_valid_position(pos.row + 1, pos.col)) 
        valid_positions.push_back({pos.row + 1, pos.col});

    // Se não há caminhos válidos, retorna
    if (valid_positions.empty()) {
        return;
    }

    // Pega o primeiro caminho para explorar na thread atual
    Position next_pos = valid_positions.back();
    valid_positions.pop_back();

    // Cria threads para os caminhos adicionais
    std::vector<std::thread> threads;
    for (const auto& p : valid_positions) {
        threads.emplace_back(walk, p);
    }

    // Explora o primeiro caminho na thread atual
    walk(next_pos);

    // Aguarda as threads filhas terminarem
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_labirinto>" << std::endl;
        return 1;
    }

    Position initial_pos = load_maze(argv[1]);
    if (initial_pos.row == -1 || initial_pos.col == -1) {
        std::cerr << "Posição inicial não encontrada no labirinto." << std::endl;
        return 1;
    }

    // Inicia a exploração
    walk(initial_pos);

    if (exit_found) {
        std::cout << "Saída encontrada!" << std::endl;
    } else {
        std::cout << "Não foi possível encontrar a saída." << std::endl;
    }

    return 0;
}
