#!/usr/bin/env python3

# Extracts test cases from a file and then
# compiles the file using the supplied w++ binary path,
# it then compares the two and if they are not the same
# we exit with non-zero status.

import sys
import re
import os
import subprocess

from operator import itemgetter


# Run wot++ with test file.
def run(args):
	return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout


if __name__ == "__main__":
	if len(sys.argv) != 3:
		print("usage: <test.wpp> <w++ exe>")
		sys.exit(1)

	_, test_file, binary = sys.argv

	# What is the correct output expected to be?
	correct_output = run([binary, test_file]).decode("UTF-8").split('\n')

	with open(test_file, 'r') as f:
		i = 0

		for j, line in enumerate(f.readlines()):
			m = re.search("(?<=\#\[ expect: ).*(?= \])", line.rstrip())

			if m is not None:
				if m.group(0) != correct_output[i]:
					print(f"mismatch, got '{correct_output[i]}', expected '{m.group(0)}' (line {j + 1})")
					sys.exit(1)

				i += 1
