#!/usr/bin/env python3

import random
import sys
#import progressbar

TEXT_SIZE = (30, 1500)
NTEXTS = (10, 200)
ALPHABET = ['A', 'T', 'G', 'C']


def gen_text():
    size = random.randint(TEXT_SIZE[0], TEXT_SIZE[1])
    return '{} {}'.format(size, ''.join(random.choices(ALPHABET, k=size)))

def gen_problem():
    n_texts = random.randint(NTEXTS[0], NTEXTS[1])
    print(n_texts)
    for _ in range(n_texts):
        print(gen_text())

if __name__ == '__main__':
    #for t in progressbar.progressbar(range(NTEXTS)):
        #print(gen_text())

    gen_problem()
