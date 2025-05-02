#!/usr/bin/env python3
"""   """
# pylint: disable=bad-indentation,line-too-long,invalid-name

import logging
import sys
import os

LOGLEVEL = os.environ.get('LOGLEVEL', 'WARN').upper()
logging.basicConfig(level=LOGLEVEL, format="%(message)s")
logger = logging.getLogger(__name__)


def lce(s, i, j):
	"""Longest common extension"""
	n = len(s)
	if i == j:
		return n - i
	if i > j:
		i, j = j, i

	k = 0
	while i + k < n and j + k < n and s[i + k] == s[j + k]:
		k += 1
	return k

def lz78_factorization(filename):
	with open(filename, 'rb') as f:
		text = f.read()

	trie = {}  # binary string -> index
	index = 1
	i = 0
	n = len(text)
	count = 0
	lcecandidate = 0
	lcelength = 0

	factors = []
	factor_startingposition = 0
	while i < n:
		prefix = []
		while i < n and bytes(prefix + [text[i]]) in trie:
			prefix.append(text[i])
			lcecandidate = trie[bytes(prefix)]
			lcelength = len(prefix) + lce(text, factor_startingposition, i+1)
			i += 1

		logger.info(f'trie leaf factor candidate: {bytes(prefix)}')
		logger.info(f'lcecandidate: {lcecandidate}, lcelength: {lcelength}, {text[factor_startingposition:factor_startingposition+lcelength]}')
		# Add new factor
		if len(prefix) < lcelength:
			factor = text[factor_startingposition:factor_startingposition+lcelength]
			i = factor_startingposition + lcelength
		else:
			factor = bytes(prefix + ([text[i]] if i < n else []))
			i += 1
		logger.info(f'{factor} -> {index}')
		factors += [factor]
		if factor not in trie:
			trie[factor] = index
			# allow for truncations
			for x in range(len(factor)):
				if factor[:x] not in trie:
					trie[factor] = index

		index += 1
		count += 1
		lcecandidate = 0
		lcelength = 0
		factor_startingposition = i

	# check whether we can decode text from the dictionary:
	decoded = b''.join(factors)
	# invtrie = {v: k for k, v in trie.items()}
	# if extrafactor is not None:
	# 	assert index-1 not in invtrie, f'Trie contains index {index-1} twice: {extrafactor} <-> {invtrie[index-1]}'
	# 	invtrie[index-1] = extrafactor
	# for i in range(1, index):
	# 	assert i in invtrie, f'Trie does not contain index {i}'
	# 	decoded += invtrie[i]
	assert len(decoded) == n, f"Decoded text length does not match original text length: {len(decoded)} != {n}"
	assert decoded == text, f"Decoded text does not match original text at character {decoded.find(text)}: {decoded} != {text}"
	return count

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print("Usage: python lz78.py <file>")
		sys.exit(1)

	print(f'RESULT file={sys.argv[1]} algo=78r count={lz78_factorization(sys.argv[1])}')

