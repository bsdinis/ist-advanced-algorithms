#!/usr/bin/env python3
from suffix_tree import *
import sys
import itertools

if __name__ == '__main__':
    strings = list()
    d = int(sys.stdin.readline())
    for line in sys.stdin.readlines():
        size, string = line.strip().split()
        size = int(size)
        strings.append(string)

    lens = list()
    for length in range(2, d+1):
        # This is a hack, we should probably change suffix_tree.py to receive the number of substrings
        max_len = 0
        for sset in itertools.combinations(strings, r=length):
            tree = SuffixTree(sset)
            max_len = max(max_len, len(tree.lcs()[0]))
        lens.append(max_len)

    print(' '.join(str(l) for l in lens))
