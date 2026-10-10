# Core-AGI: The Silicon Core

> *"In the beginning, there was only the void, a blank terminal screen, and the relentless will to architect intelligence from raw silicon."*

## Abstract

**Core-AGI** is an uncompromising, zero-dependency (`nostdlib`) autonomous artificial intelligence core engineered from scratch in C++20/23 and CUDA. It bypasses the bloated abstractions of modern software ecosystems, dropping straight down to raw Linux system calls (`mmap`, `sys_write`), low-level pointer arithmetic, and parallel GPU tensor execution.

This repository represents **Phase 1: The Silicon Core**. The foundational engine designed to run at the absolute bare-metal level, optimized for speed, precision, and autonomous code evolution.

## Architecture & Module Breakdown

* **`python.hpp`**: Created to implement Python-like utility methods and conveniences that do not exist natively in C++, achieving zero-cost abstraction while operating directly via low-level system calls and custom arena allocators.
* **`lisp.hpp`**: Created to inject Lisp logic, S-expression parsing, and functional methods directly into C++, serving as a dialectical filter for internal rules and AST execution.
* **`math.hpp`**: A foundational mathematical library providing optimized numerical primitives, bitwise operations, and safe memory ranges—actively structured for continuous development and mathematical expansion.
* **`tensor.hpp` & `cuda_engine.cu`**: High-performance CPU/GPU tensor operations, custom gradient descent algorithms (SGD, Adam), and quantile loss engines accelerated by custom CUDA kernels.

## Philosophy & Manifest

We reject bloated runtimes, garbage collectors, and unnecessary software abstraction layers. This project is built for those who wish to strip away the noise of modern computing and converse directly with silicon. The core is engineered to compute, optimize, and evolve through algorithmic tension and raw computational will.

## License

This project is protected under the **GNU General Public License v3.0 (GPLv3)**. See the `LICENSE` file for details.
