#!/usr/bin/env python3
"""
Modified from tudocomp [https://github.com/tudocomp/tudocomp] etc/compare.py
Licensed under the Apache License, Version 2.0
"""
import argparse
import collections
import hashlib
import itertools
import os
import re
import sys
import statistics
import subprocess
import tempfile
import time
import pprint
import json
from collections import namedtuple

# Argument parser
parser = argparse.ArgumentParser(description="""
    Compare running times and memory usage of a set of compressors.
""")

parser.add_argument('--iterations', '-n', type=int, default=1,
                    help='the amount of iterations for each input file')
parser.add_argument('files', metavar='FILE', type=str, nargs='+',
                    help='the input files to use for comparison')
parser.add_argument('--format', type=str, default='stdout',
                    help='Format to output')
parser.add_argument('--nomem', action="store_true",
                    help='Don\'t measure memory')

args = parser.parse_args()

class StdOutTable:
    def __init__(self):
        pass
    def print(self, *s):
        print(*s)
    def file(self, srcfname, srcsize, srchash):
        print()
        print("File: %s (%s, sha256=%s)" % (srcfname, memsize(srcsize), srchash))
    def header(self, tup):
        print()
        print(("%"+ str(maxnicknamelength) + "s | %10s | %10s |") % tup)
        print('-'*(maxnicknamelength+5*10+6*3+4+2))
    def cell(self, content, format, sep, f):
        print((format + " " + sep) % f(content), end='',flush=True)
    def end_row(self):
        print()
    def flush(self):
        pass

class JsonTable:
    def __init__(self):
        self.messages = []
        self.files = {}
    def print(self, *s):
        self.messages.append(" ".join(map(str, iter(s))))
    def file(self, srcfname, srcsize, srchash):
        self.files[srcfname] = {}
        self.currentfile = self.files[srcfname]
        self.currentfile["cols"] = {}
        self.currentfile["size"] = srcsize
        self.currentfile["hash"] = srchash
    def header(self, tup):
        self.headings = tup
        self.current_heading = 0
        for heading in tup:
            self.currentfile["cols"][heading] = []
    def cell(self, content, format, sep, f):
        self.currentfile["cols"][self.headings[self.current_heading]].append(content)
        self.current_heading += 1
    def end_row(self):
        self.current_heading = 0

    def flush(self):
        print(json.dumps(self.__dict__, sort_keys=True, indent=4))

sot = StdOutTable()
if args.format == "json":
    sot = JsonTable()

# Ensure that the input files are readable
for srcfname in args.files:
    if not os.access(srcfname, os.R_OK):
        sot.print("ERROR: Input file not found or not readable:", srcfname)
        quit()

# Check that valgrind is available for memory measurement
mem_available = False
if not args.nomem:
    try:
        subprocess.check_call(["valgrind", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        mem_available = True
    except:
        mem_available = False
        sot.print("WARNING: valgrind not found - memory measurement unavailable.")
        sot.print()

# Program execution definition
StdOut = 0
StdIn  = 0

Exec = collections.namedtuple('Exec', ['args', 'outp', 'inp'])
Exec.__new__.__defaults__ = (None, None) # args is required

# Compressor Pair definition
CompressorPair = collections.namedtuple('CompressorPair', ['name', 'compress'])

def LzdrComp(name, algorithm, lzdr_comp_binary='./target/release/lzdr-comp'):
    return CompressorPair(name,
        compress   = Exec(args=[lzdr_comp_binary, '-a', algorithm], inp=StdIn, outp=StdOut))

def LzdComp(name, algorithm, lzd_binary='./subprojects/lzd/out/lzd'):
    return CompressorPair(name,
        compress   = Exec(args=[lzd_binary, '-a', algorithm], inp='-f', outp='-o'))

def Tudocomp(name, algorithm, tdc_binary='./subprojects/tudocomp/build/tdc', cflags=[], dflags=[]):
    return CompressorPair(name,
        compress   = Exec(args=[tdc_binary, '-a', algorithm] + cflags, outp='--output'))

def StdCompressor(name, binary, cflags=[], dflags=[]):
    return CompressorPair(name,
        compress   = Exec(args=[binary] + cflags, inp=StdIn, outp=StdOut))

# Define suite
suite = [
    # lzdr-comp
    LzdrComp(name='lzdr', algorithm='lzdr'),
    LzdrComp(name='lzd+', algorithm='lzd+'),

    # LZD
    LzdComp(name='lzd', algorithm='lzd'),

    # Tudocomp
    Tudocomp(name='lz78',                                    algorithm='lz78(coder=ascii)'),
    Tudocomp(name='lzw',                                     algorithm='lzw(coder=ascii)'),

    # Some standard Linux compressors
    # StdCompressor(name='gzip -1',  binary='gzip',  cflags=['-1'], dflags=['-d']),
    # StdCompressor(name='gzip -9',  binary='gzip',  cflags=['-9'], dflags=['-d']),
    # StdCompressor(name='bzip2 -1', binary='bzip2', cflags=['-1'], dflags=['-d']),
    # StdCompressor(name='bzip2 -9', binary='bzip2', cflags=['-9'], dflags=['-d']),
    # StdCompressor(name='lzma -1',  binary='lzma',  cflags=['-1'], dflags=['-d']),
    # StdCompressor(name='lzma -9',  binary='lzma',  cflags=['-9'], dflags=['-d']),
]

def memsize(num, suffix='B'):
    for unit in ['','Ki','Mi','Gi','Ti','Pi','Ei','Zi']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'Yi', suffix)

def timesize(num, suffix='s'):
    if(num < 1.0):
        for unit in ['','m','mu','n']:
            if num > 1.0:
                return "%3.1f%s%s" % (num, unit, suffix)
            num *= 1000
        return "%.1f%s%s" % (num, '?', suffix)
    else:
        if(num < 600):
            return "%3.1f%s" % (num, 's')
        elif(num < 3600):
            num /= 60
            return "%3.1f%s" % (num, 'min')
        elif(num > 3600):
            num /= 3600
            return "%3.1f%s" % (num, 'h')

def run_exec(x, infilename, outfilename):
    args = list(x.args)

    # Delete existing output file
    if os.path.exists(outfilename) and outfilename != r"/dev/null":
        os.remove(outfilename)

    # Determine Output
    if(x.outp == StdOut):
        outfile = open(outfilename, "wb") if outfilename != r"/dev/null" else None
        pipe_out = outfile if outfilename != r"/dev/null" else subprocess.DEVNULL
    else:
        outfile = None
        pipe_out = logfile
        args += ([x.outp, outfilename] if x.outp != None else [outfilename])

    # Determine input
    if(x.inp == StdIn):
        infile = open(infilename, "rb")
        pipe_in = infile
    else:
        infile = None
        pipe_in = None
        args += ([x.inp, infilename]   if x.inp  != None else [infilename])

    # Call
    t0 = time.time()
    subprocess.check_call(args, stdin=pipe_in, stdout=pipe_out, stderr=logfile)

    # Close files
    outfile.close() if outfile else None
    infile.close()  if infile  else None

    # Yield time delta
    return(time.time() - t0)

def measure_time(x, infilename, outfilename):
    t=[]
    for _ in range(0, args.iterations):
        t = t + [run_exec(x, infilename, outfilename)]

    return(statistics.median(t))

def measure_mem(x, infilename, outfilename):
    massiffilename=tempfile.mktemp()

    run_exec(
        Exec(args=['valgrind', '-q', '--tool=massif', '--pages-as-heap=yes',  '--massif-out-file=' + massiffilename] + x.args, inp=x.inp, outp=x.outp),
        infilename, outfilename)

    with open(massiffilename) as f:
        maxmem=0
        for line in f.readlines():
            match = re.match('^mem_heap_B=([0-9]+)', line)
            if match:
                maxmem = max(maxmem,int(match.group(1)))

    os.remove(massiffilename)
    return(maxmem)

maxnicknamelength = len(max(suite, key=lambda p: len(p.name))[0] ) + 3

sot.print("Number of iterations per file: ", args.iterations)

for srcfname in args.files:
    srchash = hashlib.sha256(open(srcfname, 'rb').read()).hexdigest()
    srcsize = os.path.getsize(srcfname)

    sot.file(srcfname, srcsize, srchash)

    sot.header(("Compressor", "C Time", "C Memory"));

    logfilename = tempfile.mktemp()
    outfilename = r"/dev/null"

    def print_column(content, format="%11s", sep="|", f=lambda x:x):
        sot.cell(content, format, sep, f)
    def end_row():
        sot.end_row()

    try:
        with open(logfilename,"wb") as logfile:
            for c in suite:
                # nickname
                print_column(c.name, "%"+ str(maxnicknamelength) +"s")

                # compress time
                try:
                    comp_time=measure_time(c.compress, srcfname, outfilename)
                    print_column(comp_time*1000, f=lambda x: timesize(x/1000))
                except FileNotFoundError as e:
                    print_column("(ERR)", sep=">")
                    sot.print(" " + e.strerror)
                    continue

                # compress memory
                if mem_available:
                    comp_mem=measure_mem(c.compress, srcfname, outfilename)
                    print_column(comp_mem,f=memsize)
                else:
                    print_column("(N/A)")

                # EOL
                end_row()
    except:
        sot.print()
        sot.print("ERROR:", sys.exc_info()[0])
        sot.print(sys.exc_info()[1])

with open(logfilename, 'r') as fin: sot.print(fin.read())
os.remove(logfilename)

if os.path.exists(outfilename) and outfilename != r"/dev/null":
    os.remove(outfilename)

sot.flush()
