# lzdr-comp

Warning: only tested on Linux.

## Dependencies
### Build Dependencies
* For lzdr-comp: CMake, Make
* For lzd: SCons, clang
* For [tudocomp](https://tudocomp.github.io/documentation/index.html#building): CMake, Make, Python 3

### Justfile Dependencies
* Basic dependencies: just, git, jq, wget
* Additional dev dependencies: include-what-you-use, tokei

### Dependency Installation

- Arch Linux: `pacman -S cmake make python git jq wget just scons clang valgrind`

## Usage
### Build lzdr-comp
Release mode:
```
just build-release
```

Debug mode:
```
just build-debug
```

Executables are located at `target/release/lzdr-comp` and `target/debug/lzdr-comp` after the build process.

- The executables expect input to parse from `<STDIN>`
- To compute the number of factors of all implemented algorithms, run one of the executables with parameter `--factors`
- To restrict the computation to LZD+/LZDR, run with `-a [lzd+|lzdr]` (you need to write lzd or lzd+ in lower case)
- `target/debug/lzdr-comp` also outputs verbosely the constructed factors

### Build subprojects
Build lzd and tudocomp:
```
just build-subprojects
```

### Download Pizza&Chili dataset
```
just download-pizza-chili-dataset
```
Saved into `datasets` directory.

### Factor count comparison
```
just factor-count <FILE>
```

### Calgary and Canterbury factor counts
```
just factor-count-ca-bench
```

### Pizza&Chili factor counts
```
just factor-count-pizza-chili-bench
```

### Execution time and maximum memory usage
Use `bench_speed_mem.py`. Requires Python 3 and Valgrind.
