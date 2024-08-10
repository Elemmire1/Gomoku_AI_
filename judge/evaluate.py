'''
running example:
python evaluate.py --agents-path to/your/agent/path to/another/agent/path --num-plays 64 --num-workers 16
NOTE: AT LEAST one of your testing agents needs to invite RANDOMNESS.
'''


import argparse
import numpy as np
import subprocess
import ray

from typing import Sequence
from dataclasses import dataclass


class Board:
    '''
    A gomoku board, 15x15, 1 denotes black, -1 denotes white, 0 denotes empty.
    '''
    board: np.ndarray
    BLACK_WIN: int
    WHITE_WIN: int
    DRAW: int
    NOT_END: int
    filters: list[np.ndarray]

    def __init__(self) -> None:
        self.board = np.zeros((15, 15), dtype='int32')
        self.BLACK_WIN = 1
        self.WHITE_WIN = -1
        self.DRAW = 0
        self.NOT_END = 999
        self.filters = [
            np.ones((5, 1), dtype='int32'),
            np.ones((1, 5), dtype='int32'),
            np.eye(5, 5, dtype='int32'),
            np.flip(np.eye(5, 5, dtype='int32'), axis=0),
        ]

    def flip(self) -> None:
        self.board *= -1

    def __getitem__(self, idx: tuple[int, int]) -> np.int32:
        return self.board[idx]
    
    def __setitem__(self, idx: tuple[int, int], val: int | np.int32) -> None:
        self.board[idx] = val

    def reset(self) -> None:
        self.board[:] = 0
    
    def check_end(self) -> int:
        def check(board: np.ndarray, kernel: np.ndarray) -> np.ndarray:
            assert board.ndim == kernel.ndim == 2
            result_shape = (15 + kernel.shape[0] - 1, 15 + kernel.shape[1] - 1)
            result = np.zeros(result_shape, dtype='int32')
            for i in range(kernel.shape[0]):
                for j in range(kernel.shape[1]):
                    result[i : i + 15, j : j + 15] += board * kernel[i, j]
            if result.max() >= 5:
                return self.BLACK_WIN
            if result.min() <= -5:
                return self.WHITE_WIN
            if np.count_nonzero(board) >= 225:
                return self.DRAW
            return self.NOT_END
        
        for _filter in self.filters:
            check_result = check(self.board, _filter)
            if check_result != self.NOT_END:
                return check_result
        return self.NOT_END


@dataclass
class EvaluationResult:
    num_plays: int
    wins: Sequence[int]     # w.r.t. to the agents
    agents_path: Sequence[str] | None = None

    def __post_init__(self) -> None:
        assert len(self.wins) == 2
        if self.agents_path is not None:
            assert len(self.agents_path) == 2 

    def __add__(self, other: 'EvaluationResult') -> 'EvaluationResult':
        assert (
            self.agents_path is None or other.agents_path is None
            or list(self.agents_path) == list(other.agents_path)
        ), f'only EvaluationResult for same agents can be added'

        return EvaluationResult(
            self.num_plays + other.num_plays,
            [self.wins[0] + other.wins[0], self.wins[1] + other.wins[1]],
            self.agents_path if self.agents_path is not None else other.agents_path
        )
    
    def get_scores(self) -> list[float]:
        assert self.num_plays > 0
        score = (self.wins[0] - self.wins[1]) / self.num_plays
        return [score, -score]
    
    def summary(self) -> None:
        assert self.agents_path is not None
        scores = self.get_scores()
        print('************ Summary ************')
        print(f'num plays: {self.num_plays}')
        print(f'Agent {self.agents_path[0]}:\nscore = {scores[0]} | wins = {self.wins[0]}')
        print(f'Agent {self.agents_path[1]}:\nscore = {scores[1]} | wins = {self.wins[1]}')
    

class Judge:
    def __init__(self, black_path: str, white_path: str) -> None:
        self.black_path = black_path
        self.white_path = white_path
        self.board = Board()
        self.procs: list[subprocess.Popen] = []

    def __call__(self) -> int:
        self.procs = [
            subprocess.Popen(self.black_path, stdin=subprocess.PIPE, stdout=subprocess.PIPE),
            subprocess.Popen(self.white_path, stdin=subprocess.PIPE, stdout=subprocess.PIPE),
        ]

        def send(proc: subprocess.Popen, message: str) -> None:
            value = f'{message}\n'
            value = bytes(value, 'UTF-8')
            proc.stdin.write(value)
            proc.stdin.flush()
        def receive(proc: subprocess.Popen) -> str:
            return proc.stdout.readline().strip().decode()
    
        for i, proc in enumerate(self.procs):
            send(proc, i)
            _ = receive(proc)
        
        def receive_as_action(proc: subprocess.Popen) -> tuple[int, int] | None:    # None denotes output error
            value = receive(proc).split(' ')
            if len(value) != 2:
                return None
            if not value[0].isdigit() or not value[1].isdigit():
                return None
            return int(value[0]), int(value[1])
        def send_action(proc: subprocess.Popen, action: tuple[int, int]) -> None:
            send(proc, f'{action[0]} {action[1]}')
        
        def close_all() -> None:
            for proc in self.procs:
                proc.terminate()
        
        send_action(self.procs[0], (-1, -1))
        i = 0
        while True:
            action = receive_as_action(self.procs[i])
            if action is None:
                close_all()
                return self.board.WHITE_WIN if i == 0 else self.board.BLACK_WIN
            elif action == (-1, -1):
                self.board.flip()
            else:
                self.board[action] = 1 if i == 0 else -1
            end = self.board.check_end()
            if end != self.board.NOT_END:
                close_all()
                return end
            i = 1 - i
            send_action(self.procs[i], action)


def evaluate(
    agents_path: Sequence[str],
    num_plays: int,
    which_black_first: int,  
) -> EvaluationResult:
    assert len(agents_path) == 2
    assert num_plays > 0
    assert which_black_first in {0, 1}

    which_black = which_black_first
    wins = [0, 0]

    for _ in range(num_plays):
        play_result = Judge(agents_path[which_black], agents_path[1 - which_black])()
        if play_result == 1:
            wins[which_black] += 1
        elif play_result == -1:
            wins[1 - which_black] += 1
        else:
            ...
        which_black = 1 - which_black
       
    return EvaluationResult(num_plays, wins, agents_path)




def main(
    agents_path: Sequence[str],
    num_plays: int,
    num_workers: int,
) -> None:
    assert len(agents_path) == 2
    assert num_plays > 0
    assert num_workers > 0

    ray.init(num_cpus=num_workers)
    num_plays_for_worker = [num_plays // num_workers + (1 if i < num_plays % num_workers else 0) for i in range(num_workers)]
    which_black_first = [sum(num_plays_for_worker[:i]) % 2 for i in range(num_workers)]
    results = ray.get([
        ray.remote(evaluate).remote(agents_path, num_plays_for_worker[i], which_black_first[i])
        for i in range(num_workers)
    ])
    final_result: EvaluationResult = sum(results, EvaluationResult(0, [0, 0], agents_path))
    final_result.summary()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--agents-path', nargs=2, type=str, required=True)
    parser.add_argument('--num-plays', type=int, required=True)
    parser.add_argument('--num-workers', type=int, default=16)
    args = parser.parse_args()

    assert len(args.agents_path) == 2
    assert args.num_plays > 0
    assert args.num_workers > 0

    main(args.agents_path, args.num_plays, args.num_workers)
