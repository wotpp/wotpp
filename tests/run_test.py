#!/usr/bin/env python3

import sys, re, os, subprocess
from operator import itemgetter

def run(args):
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout

if __name__ == '__main__':
    actual_output = run([sys.argv[2], sys.argv[1]]).decode('UTF-8').split('\n')

    with open(sys.argv[1], 'r') as f:
        i = 0
        for j, line in enumerate(f.readlines()):
            m = re.search('(?<=\#\[ expect: ).*(?= \])', line.rstrip())
            if m != None:
                if m.group(0) != actual_output[i]:
                    print('Unexpected output from wot++ "{}", expected #{} "{}" (line {})'
                          .format(actual_output[i], i+1, m.group(0), j))
                    sys.exit(1)

                i += 1
