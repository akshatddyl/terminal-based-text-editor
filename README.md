# Terminal Based Text Editor with Advanced Data Structures

A lightweight, efficient terminal-based text editor built in C, showcasing advanced data structures like **Rope**, **Stack**, and **ncurses** for a smooth editing experience.

![C](https://img.shields.io/badge/Language-C-blue)
![License](https://img.shields.io/badge/License-MIT-green)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20WSL2-lightgrey)

---

## 📋 Table of Contents
- [Features](#features)
- [Demo](#demo)
- [Installation](#installation)
- [Usage](#usage)
- [Data Structures Used](#data-structures-used)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Team](#team)

---

## ✨ Features

✅ **Rope Data Structure** - Efficient text buffer for fast insertions and deletions  
✅ **Undo/Redo System** - Stack-based operation history (up to 50 operations)  
✅ **File Operations** - Open, edit, and save text files  
✅ **Terminal UI** - Clean ncurses-based interface  
✅ **Real-time Status Bar** - Shows filename, cursor position, and modified status  
✅ **Cross-platform** - Works on Linux, macOS, and Windows (via WSL2)

---

## 🎬 Demo
```bash
# Open an existing file
./texteditor myfile.txt

# Or start with a blank document
./texteditor
```

### Screenshot
```
┌─────────────────────────────────────────────────────────┐
│ Hello, World!                                           │
│ This is a rope-based text editor.                       │
│ Try editing, undo (Ctrl+Z), and redo (Ctrl+Y)!          │
│                                                         │
│                                                         │
└─────────────────────────────────────────────────────────┘
 myfile.txt [+] | Ln 3, Col 47 | ^S:Save ^Q:Quit ^Z:Undo ^Y:Redo
```

---

## 🚀 Installation

### Prerequisites
- **GCC Compiler** (or any C compiler)
- **ncurses library**

#### Install ncurses on Linux (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install libncurses5-dev libncursesw5-dev
```

#### Install ncurses on macOS:
```bash
brew install ncurses
```

#### Install ncurses on Windows (WSL2):
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```

### Build the Project
```bash
# Clone the repository
git clone https://github.com/yourusername/text-editor.git
cd text-editor

# Compile the project
make

# Run the editor
./texteditor
```

---

## 📖 Usage

### Basic Commands
```bash
# Open a file
./texteditor filename.txt

# Create a new file
./texteditor
```

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Arrow Keys` | Move cursor |
| `Ctrl + S` | Save file |
| `Ctrl + Q` | Quit editor |
| `Ctrl + Z` | Undo last operation |
| `Ctrl + Y` | Redo operation |
| `Backspace` | Delete character before cursor |
| `Enter` | Insert new line |

---

## 🧠 Data Structures Used

### 1. **Rope (Text Buffer)**
A rope is a binary tree where each leaf contains a string and each internal node stores the weight (total characters in the left subtree). This allows:
- ✅ **O(log n)** insertion and deletion
- ✅ Efficient handling of large files
- ✅ Better than arrays for frequent edits

**Operations:**
- `rope_insert()` - Insert text at any position
- `rope_delete()` - Delete range of characters
- `rope_concat()` - Join two ropes
- `rope_to_string()` - Convert rope to displayable string

### 2. **Stack (Undo/Redo)**
A LIFO stack stores edit operations for undo/redo functionality:
- ✅ **O(1)** push and pop operations
- ✅ Stores operation type, position, and affected text
- ✅ Limited to 50 operations (configurable)

**Operations:**
- `undo_push()` - Save an operation
- `undo_pop()` - Retrieve and undo last operation
- `redo_push()` - Save undone operation for redo

### 3. **ncurses (Terminal UI)**
Low-level terminal manipulation library for:
- ✅ Cursor positioning
- ✅ Real-time key input
- ✅ Screen refresh and display

---
## 👥 Team

**PBL-DS-TEAM**

| Name | Role | Responsibility |
|------|------|----------------|
| **Akshat Dhondiyal** | Team Lead | Terminal UI, Trie implementation, spell-checking |
| **Shivansh Vats** | Developer | Hash table for syntax highlighting |
| **Apoorv Rawat** | Developer | Rope buffer design |
| **Harsh Kumar** | Developer | Undo/redo stack, search/replace |

**Course:** Data Structures in C  
**Institution:** Graphic Era Hill University

---

## 🐛 Known Issues

- Horizontal scrolling not yet implemented (long lines wrap)
- Limited to basic ASCII characters
- No mouse support
- Status bar may truncate on very small terminals
---

## 📧 Contact

For questions or feedback, reach out to:
- **Akshat Dhondiyal** - akshatdhondiyal14@gmail.com

---

<div align="center">
Made by PBL-DS-TEAM
</div>
