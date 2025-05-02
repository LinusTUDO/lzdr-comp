#!/usr/bin/env python3
"""   """
# pylint: disable=bad-indentation,line-too-long,invalid-name

import logging
import sys

import os

LOGLEVEL = os.environ.get('LOGLEVEL', 'WARN').upper()
logging.basicConfig(level=LOGLEVEL, format="%(message)s")


logger = logging.getLogger(__name__)


def lz78_factorization(filename):
	with open(filename, 'rb') as f:
		text = f.read()

	trie = {}  # binary string -> index
	index = 1
	i = 0
	n = len(text)
	count = 0
	extrafactor = None

	while i < n:
		prefix = []
		while i < n and bytes(prefix + [text[i]]) in trie:
			prefix.append(text[i])
			i += 1

		logger.info(f'trie leaf factor candidate: {bytes(prefix)}')
		# Add new factor
		factor = bytes(prefix + ([text[i]] if i < n else []))
		i += 1
		logger.info(f'{factor} -> {index}')
		if factor in trie:
			assert extrafactor is None, 'Trie contains factor twice: {factor} <-> {extrafactor}'
			assert i >= n, f'extra factor condition not met: {i} != {n}'
			extrafactor = factor
		else:
			trie[factor] = index
		index += 1
		count += 1

	# check whether we can decode text from the dictionary:
	decoded = bytearray()
	invtrie = {v: k for k, v in trie.items()}
	if extrafactor is not None:
		assert index-1 not in invtrie, f'Trie contains index {index-1} twice: {extrafactor} <-> {invtrie[index-1]}'
		invtrie[index-1] = extrafactor
	for i in range(1, index):
		assert i in invtrie, f'Trie does not contain index {i}'
		decoded += invtrie[i]
	assert len(decoded) == n, f"Decoded text length does not match original text length: {len(decoded)} != {n}"
	assert decoded == text, f"Decoded text does not match original text at character {decoded.find(text)}: {decoded} != {text}"
	return count

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print("Usage: python lz78.py <file>")
		sys.exit(1)

	print(f'RESULT file={sys.argv[1]} algo=78 count={lz78_factorization(sys.argv[1])}')

