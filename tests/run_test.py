#!/usr/bin/env python3

# Extracts test cases from a file and then
# compiles the file using the supplied w++ binary path,
# it then compares the two and if they are not the same
# we exit with non-zero status.

import sys
import ast
import re
import os
import subprocess

from operator import itemgetter


# Run wot++ with test file.
def run(args):
	res = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	output = res.stdout.decode("UTF-8")

	if res.returncode != 0:
		raise RuntimeError(f"status({res.returncode}) `{output[:-1]}`")

	return output


if __name__ == "__main__":
	if len(sys.argv) != 3:
		print("usage: <test.wpp> <w++ exe>")
		sys.exit(1)

	# Unpack argv
	_, test_file, binary = sys.argv


	# Run wot++ and get output.
	wpp_output = ""

	try:
		wpp_output = run([binary, test_file])
		if wpp_output[-1] == '\n':
			wpp_output = wpp_output[:-1]

	except RuntimeError as err:
		print(f"w++ failed: {err.args[0]}")
		sys.exit(1)

	# Find all test cases of the form `#[expect(foo)]`
	matches = []

	with open(test_file, 'r') as f:
		test_file = f.read()

	match_iter = re.finditer(r"(?:#\[\s*expect\()(((.+)|\n|\t)?)(?:\)\s*\])", test_file)

	if match_iter is not None:
		for m in match_iter:
			matches.append(m)


	if len(matches) == 0:
		print("no test cases defined!")
		sys.exit(1)


	# Compare the expected output to the actual output.
	ptr = 0
	for m in matches:
		test = m.group(1).encode('latin-1', 'backslashreplace').decode('unicode-escape')
		length = len(test)

		actual = wpp_output[ptr : ptr + length]


		# Check for mismatch.
		if actual != test:
			start, end = m.span()   # Byte offset of comparison that failed.
			line = column = 1


			# Loop through characters in `test_file` and keep track of byte offset.
			for byte_offset, char in enumerate(test_file):
				# Break if we're at the location of the comparison that failed.
				if byte_offset == start:
					break

				# Track line number and column.
				if char == '\n':
					line += 1
					column = 1

				else:
					column += 1


			# Print error location and the error itself.
			print(f"expect @ {line}:{column} failed!")
			print(f" -> expected '{actual}', got '{test}'.")


			# Return non-zero status.
			sys.exit(1)


		# Advance window by length of `test`.
		ptr += length





