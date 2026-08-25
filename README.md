# Aurum
![Latest Tag](https://img.shields.io/github/v/tag/lishacodesgames/Aurum?color=%237DBA84)
![Status Badge](https://img.shields.io/badge/Status-In_Development-yellow)

## Overview
A compiler for my own programming language! <br>
The programming language is called Aurum, inspired by the latin word for Gold

### Tech Stack
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![AssemblyScript](https://img.shields.io/badge/assembly%20script-%23000000.svg?style=for-the-badge&logo=assemblyscript&logoColor=white)
![Bash Script](https://img.shields.io/badge/bash_script-%23121011.svg?style=for-the-badge&logo=gnu-bash&logoColor=white) 

## Syntax
I code in C++, Java and Python, so I want to mix all the best syntax features (in my humble opinion, of course) from them into this language

- File extension: `.aura`
- Semicolon based

## Architecture
currently only supports MacOS, running assembly in x64 architecture, bcz I like its syntax better
I do want to support Linux soon... I'll see about Windows

Currently, `scripts/` only holds the mac's compilation command for assembly, but I hope to add more... soon...

## Build & Run
(Run from `Aurum/`)

### Build
```bash
cmake --preset Release
cmake --build --preset Release
```

### Run
```bash
./build/Release/Aurum foundry/gold.aura
```

### Dependencies
- NASM
