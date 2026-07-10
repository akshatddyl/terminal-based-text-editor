# Terminal-Based Text Editor

[![C](https://img.shields.io/badge/language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![ncurses](https://img.shields.io/badge/UI-ncurses-green.svg)](https://invisible-island.net/ncurses/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Demo
<img width="1904" height="1026" alt="Screenshot From 2026-07-10 14-30-50" src="https://github.com/user-attachments/assets/d9eab290-9e3e-458e-929e-ec6ac755d5e1" />


## Project Overview

This project is a lightweight, high-performance, terminal-native text editor written entirely in C from scratch. Designed with a focus on low-level systems programming, it entirely eschews external libraries (other than standard C libraries and `ncurses` for the UI UI) in favor of custom-built, optimized data structures. The editor is built to handle massive text files gracefully while providing an IDE-like experience directly in the terminal.

## Features

- **Split-Pane Architecture:** View and edit two files side-by-side using horizontal splits.
- **Smart Auto-Indentation:** Automatically preserves the previous line's whitespace and intelligently indents after opening braces `{`.
- **Fast Search & Replace:** Integrated Boyer-Moore string matching for lightning-fast, full-document search and replace operations.
- **Auto-Suggestions:** Real-time predictive typing and keyword completion based on the current context.
- **Syntax Highlighting:** Real-time C/C++ keyword and type highlighting.
- **Robust Undo/Redo:** Flawless state tracking allowing unlimited undos and redos of both single keystrokes and massive search-and-replace queries.
- **Crash Recovery:** Atomic saves and background auto-saving to hidden `.swp` files protect against data loss.

## Architecture & Data Structures

The editor's speed and reliability stem from its meticulously crafted core data structures:

- **Rope:** The core text buffer is implemented as a binary tree (Rope) rather than a flat string array. This guarantees $O(\log N)$ time complexity for insertions and deletions, regardless of the file size, and features $O(1)$ tracking for weights, lengths, and depths.
- **Stack:** A ring-buffer stack engine powers the Undo/Redo state machine, tracking absolute character offsets and exact inverse operations (`OP_INSERT` / `OP_DELETE`).
- **Trie:** An extremely fast prefix tree (Trie) is queried on every keystroke to provide instant $O(M)$ keyword auto-suggestions.
- **Hash Table:** An optimized hash map drives the syntax highlighting engine, instantly mapping parsed string tokens to `ncurses` color pairs during the render loop.

## Installation & Build

### Prerequisites
You will need a standard C compiler (like `gcc`), `make`, and the `ncurses` development libraries.

On Fedora/RHEL:
```bash
sudo dnf install gcc make ncurses-devel
```

On Ubuntu/Debian:
```bash
sudo apt install build-essential libncurses5-dev libncursesw5-dev
```

### Compilation
Clone the repository and run `make`:
```bash
make
```

### Usage
Run the editor with one or two files:
```bash
# Single file mode
./texteditor main.c

# Split-pane mode
./texteditor main.c rope.c
```

### Keybindings
- `Ctrl+Q` - Quit
- `Ctrl+S` - Save file
- `Ctrl+F` - Find
- `Ctrl+R` - Find and Replace
- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `Ctrl+W` - Swap focus between split panes
- `Tab` - Accept auto-suggestion (or insert 4 spaces)
- `Arrows` - Navigate cursor

## Testing & Memory Safety

> [!IMPORTANT]  
> **Strict Invariant Checking and Memory Profiling**

A primary focus of this project was mathematical precision and absolute memory safety. 
- The project includes dedicated standalone test suites for the core data structures (e.g., `test_rope.c`, `test_undo.c`).
- The core Rope engine and Undo stack went through strict bounds-checking (`assert.h`) and memory profiling using **Valgrind** to ensure **zero memory leaks** and to completely eliminate segmentation faults during complex node splitting, concatenation, and iterator seeking.
- Every individual tree mutation (insert, delete, split) actively recursively validates the integrity of the total weights, string lengths, newlines, and depths before continuing.

## Benchmarks

The Rope data structure was heavily optimized for large file operations. A dedicated `benchmark.c` suite was created to stress-test the architecture against a naive string buffer (using standard `memmove`).

**Benchmark:** 100,000 random insertions and deletions exactly in the middle of a buffer.

| Data Structure | Time (100k mutations) | Complexity |
| :--- | :--- | :--- |
| **Naive String Buffer** | ~0.045s | $O(N)$ |
| **Optimized Rope Tree** | ~1.479s | $O(\log N)$ |

*Note: While the naive buffer benefits heavily from CPU vectorization on small files, its performance degrades linearly **O(N)** on massive files. The custom Rope structure scales logarithmically, making it substantially more performant as file sizes approach gigabyte boundaries.*
