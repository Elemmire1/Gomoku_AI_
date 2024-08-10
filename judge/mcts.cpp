#include "AIController.h"
#include <utility>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>

#define ratio 1
#define ratio2 1
#define MAX_SEARCHES 50000   //走每步棋之前模拟次数

extern int ai_side; //0: black, 1: white

std::string ai_name = "El's Robot";

int turn = 0;
std::vector<std::vector<int>> board(15, std::vector<int>(15, -1));   //把棋盘重新用向量表示一边

void print()
{
    for (int i = 0; i < 15; i++)
    {
        for (int j = 0; j <= 14; j++)
        {
            if (board[i][j] == -1) std::cout << "_ ";
            else std::cout << board[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

struct point {
    int first;
    int second;
    point operator+(const point& other)
    {
        point res;
        res.first = first + other.first;
        res.second = second + other.second;
        return res;
    }
    point operator-(const point& other)
    {
        point res;
        res.first = first - other.first;
        res.second = second - other.second;
        return res;
    }
};

std::vector<point> dir = { {1,0},{0,1},{1,1},{1,-1} };

struct Node {
    std::vector<std::vector<int>> current_board;  //当前棋盘状态
    point p; //下在了哪里
    Node* parent;  //他爹
    std::vector<Node*> children; //下一步棋以后能够到达的子状态
    int visits;  //访问次数
    double reward;  //经过模拟后得到的 输/赢 情况对应的奖励值
    int currentPlayer;  //下这一步棋的当前玩家

    Node(std::vector<std::vector<int>> b, Node* p = nullptr, int player = 0)
            : current_board(b), parent(p), visits(0), reward(0.0), currentPlayer(player), p(point{ -1,-1 }) {}

    ~Node() {
        for (auto child : children) {
            delete child;
        }
    }
};

int get(point p)
{
    return board[p.first][p.second];
}
void put(point p, int color)
{
    board[p.first][p.second] = color;
}
void remove(point p)
{
    board[p.first][p.second] = -1;
}

bool inboard(point p)
{
    return 0 <= p.first && p.first <= 14 && 0 <= p.second && p.second <= 14;
}

//init function is called once at the beginning
void init() {
    srand(time(0));
}

Node* choose(Node* node) {
    Node* bestChild = nullptr;
    double bestValue = -std::numeric_limits<double>::infinity();

    for (auto& child : node->children) {
        //如果一个节点从未被访问过，设置他的ucb为正无穷
        if (child->visits == 0) return child;

        double ucb1 = (child->reward / child->visits) + sqrt(2 * log(node->visits) / child->visits);
        if (ucb1 > bestValue) {
            bestValue = ucb1;
            bestChild = child;
        }
    }

    return bestChild;
}

bool adjacent(int x, int y, const std::vector<std::vector<int>>& board) {
    // 棋盘的大小
    int rows = board.size();
    int cols = board[0].size();

    // 八个方向的偏移量
    std::vector<std::pair<int, int>> directions = {
            { -1, -1 }, { -1, 0 }, { -1, 1 },
            { 0, -1 },          { 0, 1 },
            { 1, -1 }, { 1, 0 }, { 1, 1 }
    };

    for (const auto& dir : directions) {
        int newX = x + dir.first;
        int newY = y + dir.second;

        // 检查新位置是否在棋盘范围内
        if (newX >= 0 && newX < rows && newY >= 0 && newY < cols) {
            if (board[newX][newY] != -1) { // 如果该位置有棋子
                return true;
            }
        }
    }

    return false;
}

void expand(Node* node)
{
    int nextPlayer = 1 - node->currentPlayer; // 切换到下一步的玩家
    for (int x = 0; x < 15; ++x) {
        for (int y = 0; y < 15; ++y) {
            if (node->current_board[x][y] == -1 && adjacent(x, y, node->current_board))
            { // 如果该位置为空
                std::vector<std::vector<int>> new_board = node->current_board;
                // 在新的棋盘上放置新的棋子
                new_board[x][y] = nextPlayer;
                // 创建一个新的子节点
                Node* child = new Node(new_board, node, nextPlayer);
                child->p = point{ x, y };
                node->children.push_back(child);
            }
        }
    }
}

Node* random_choose(Node* node) {
    if (node->children.empty()) throw "Error: no children available for random selection";
    int index = rand() % node->children.size();
    return node->children[index];
}

bool check_winner(const std::vector<std::vector<int>>& board, int player)
{
    for (int x = 0; x <= 14; x++)
    {
        for (int y = 0; y <= 14; y++)
        {
            point p = { x,y };
            if (board[x][y] != player) continue;

            for (int i = 0; i <= 3; i++)
            {
                int length = 1;
                point r_end = p + dir[i], l_end = p - dir[i];

                while (inboard(r_end) && board[r_end.first][r_end.second] == player) {
                    length++;
                    r_end = r_end + dir[i];
                }

                while (inboard(l_end) && board[l_end.first][l_end.second] == player) {
                    length++;
                    l_end = l_end - dir[i];
                }

                if (length == 5) return true;
            }
        }
    }
    return false;
}

bool is_full(const std::vector<std::vector<int>>& board) {
    for (const auto& row : board) {
        for (int cell : row) {
            if (cell == -1) return false;
        }
    }
    return true;
}

double rollout(Node* node) {
    std::vector<std::vector<int>> temp_board = node->current_board;
    int current_player = node->currentPlayer; //现在下的人是下出这一步的人的下一个人

    std::vector<point> empty_points;
    for (int x = 0; x < 15; ++x) {
        for (int y = 0; y < 15; ++y) {
            if (temp_board[x][y] == -1) {
                empty_points.push_back(point{ x, y });
            }
        }
    }

    while (true) {
        // Check if current player wins
        if (check_winner(temp_board, current_player)) {
            return current_player == node->currentPlayer ? 1.0 : -1.0;
        }

        current_player = 1 - current_player;

        // Check if the board is full (draw)
        if (is_full(temp_board)) {
            return 0.0;
        }

        if (empty_points.empty()) {
            return 0.0; // No moves left, it's a draw
        }

        int index = rand() % empty_points.size();
        point move = empty_points[index];
        empty_points.erase(empty_points.begin() + index);
        temp_board[move.first][move.second] = current_player;
    }
}

void back_propagate(Node* node, double reward) {
    int flag = 1;
    while (node != nullptr) {
        node->visits++;     //visits在这里加过了，别的地方不用再加了
        node->reward += reward * flag;
        node = node->parent;
        flag *= -1;
    }
}



std::pair<int, int> action(std::pair<int, int> loc) {
    turn++;
    if (loc.first != -1 && loc.second != -1)
        board[loc.first][loc.second] = 1 - ai_side;   //ai_side取0或1，0先，1后， 1-ai_side就是放上一个人类的棋子

    if (loc.first == -1 && loc.second == -1 && turn != 1)
        ai_side = 1 - ai_side;    //满足人类的换边需求

    if (turn == 1 && ai_side == 0) {
        put(point{ 14,14 }, ai_side);
        return { 14,14 };
    } //第一步摆烂，防止被换边

    //这里要你下棋了啊！

    Node* current_node = new Node(board, nullptr, 1 - ai_side);   //对当前局面创建一个蒙特卡洛树（根）节点   当前这一子是人类下的
    current_node->p = point{ loc.first, loc.second };
    Node* node = current_node;
    //Node* optimal = new Node(board, nullptr, 1 - ai_side);

    for (int i = 0; i < MAX_SEARCHES; ++i)
    {
        node = current_node;
        while (!node->children.empty())
        {
            node = choose(node);
        }

        if (node->visits > 0)
        {
            expand(node);
            node = random_choose(node);
        }

        double reward = rollout(node);
        back_propagate(node, reward);
    }

        Node* optimal=current_node->children[0];
    for (auto& child : current_node->children)
        if (optimal->visits < child->visits)
        {
            optimal = child;
        }

    put(optimal->p, ai_side);
    int x = (optimal->p).first, y = (optimal->p).second;

    // 递归地释放所有节点的内存
    delete current_node;

    return std::make_pair(x, y);
}