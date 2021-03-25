#!/usr/bin/env python3

import random
import sys
import progressbar

TEXT_SIZE =    (30, 1000000)
PATTERN_SIZE = (3,  10000)
NTEXTS = 50
NPATTERNS = 500
ALGORITHMS = ['K', 'B']
ALPHABET = ['A', 'T', 'G', 'C']


def gen_text():
    size = random.randint(TEXT_SIZE[0], TEXT_SIZE[1])
    return 'T {}'.format(''.join(random.choices(ALPHABET, k=size)))

def gen_pattern():
    size = random.randint(PATTERN_SIZE[0], PATTERN_SIZE[1])
    pat = ''.join(random.choices(ALPHABET, k=size))
    return '\n'.join('{} {}'.format(algo, pat) for algo in ALGORITHMS)


if __name__ == '__main__':
    for t in progressbar.progressbar(range(NTEXTS)):
        print(gen_text())
        for p in progressbar.progressbar(range(NPATTERNS)):
            print(gen_pattern())

    print('X')
