# ref:代码框架由GPT辅助完成

from flask import Flask, request, jsonify, render_template
import subprocess
import numpy as np
import sys
import os
import signal
import time

app = Flask(__name__)

class AI:
    def __init__(self, path, id):
        self.path = path
        if path == 'human':
            self.human = 1
        else:
            self.human = 0
        self.id = id
        self.init()

    def send(self, message):
        value = str(message) + '\n'
        value = bytes(value, 'UTF-8')
        self.proc.stdin.write(value)
        self.proc.stdin.flush()
        print(f"发送给 AI: {message}")

    def receive(self):
        response = self.proc.stdout.readline().strip().decode()
        print(f"从 AI 接收到: {response}")
        return response

    # 确保 init 和 action 方法有类似的日志记录

    def init(self):
        if self.human == 0:
            self.proc = subprocess.Popen(self.path,
                                         stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
            self.send(self.id)
            self.name = self.receive()

    def action(self, a, b):
        if self.human == 1:
            value = sys.stdin.readline().strip().split(' ')
        else:
            print(f"动作调用： {a} {b}")
            self.send(str(a) + ' ' + str(b))
            value = self.receive().split(' ')
        print(f"动作结果： {value}")
        return int(value[0]), int(value[1])
def run_with_timeout(func, *args, timeout=10):
    def handler(signum, frame):
        raise TimeoutError()

    signal.signal(signal.SIGALRM, handler)
    signal.alarm(timeout)
    try:
        result = func(*args)
    finally:
        signal.alarm(0)
    return result

ai = "fuck python"

class Board:
    def __init__(self):
        self.board = -np.ones((15, 15), dtype=int)

    def action(self, side, x, y):
        self.board[x][y] = side

    def check_win(self, side, x, y):
        if x < 0 or x >= 15 or y < 0 or y >= 15 or self.board[x][y] != side:
            return False

        # 8 Directions
        directions = [(1, 0), (0, 1), (1, 1), (1, -1)]
        for dx, dy in directions:
            count = 1
            for i in range(1, 5):
                nx, ny = x + i * dx, y + i * dy
                if 0 <= nx < 15 and 0 <= ny < 15 and self.board[nx][ny] == side:
                    count += 1
                else:
                    break

            for i in range(1, 5):
                nx, ny = x - i * dx, y - i * dy
                if 0 <= nx < 15 and 0 <= ny < 15 and self.board[nx][ny] == side:
                    count += 1
                else:
                    break

            if count >= 5:
                return True
        return False

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/init', methods=['POST'])
def total_init():
    data = request.json
    ai_id=int(data.get('ai_id'))
    try:
        global ai
        ai = AI('../judge/sample', ai_id)
        return jsonify({'status': 'success'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)})

@app.route('/move', methods=['POST'])
def move():
    data = request.json
    ai_path = data.get('ai_path')
    ai_id = data.get('ai_id')
    board_state = data.get('board_state')
    last_move = data.get('last_move', (-1, -1))
    x, y = last_move

    board = Board()
    board.board = np.array(board_state)

    # Check if the player's move leads to a win
    if board.check_win(0, x, y):
        return jsonify({'status': 'success', 'move': [x, y], 'winner': 0})  #人类赢

    try:
        global ai
        new_x, new_y = ai.action(x, y)
    except TimeoutError:
        return jsonify({'status': 'error', 'message': 'AI action timeout'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': f'AI action failed: {e}'})

    board.action(1, new_x, new_y)

    if board.check_win(1, new_x, new_y):
        return jsonify({'status': 'success', 'move': [new_x, new_y], 'winner': 1})  

    return jsonify({'status': 'success', 'move': [new_x, new_y], 'winner': -1})

if __name__ == '__main__':
    app.run(debug=True)