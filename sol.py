#!/usr/bin/env python3
from suffix_tree import *
import sys
import itertools

if __name__ == '__main__':
    strings = list()
    d = int(sys.stdin.readline())
    for line in sys.stdin.readlines():
        size, string = line.strip().split()
        size = int(size) #type: ignore
        strings.append(string)

    tree = SuffixTree(strings)
    tree.correct_ids()
    with open('py_sol.dot', 'w') as f:
        tree.dot(f)

    tree.print(sys.stderr);
    print(' '.join(str(l) for l in tree.lcs_all()), end=" \n")
