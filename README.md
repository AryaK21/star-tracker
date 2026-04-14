
# 🌌 The Star Tracker
> A High-Performance, GPU-Accelerated Celestial Engine & N-Body Simulator built in pure C.

**The Dark Architect** is a capstone-level systems engineering project that simulates large-scale stellar mechanics. It parses real-world astronomical datasets, translates absolute space into local coordinates via spherical trigonometry, and renders thousands of stars in real-time using a custom OpenGL pipeline. It is architected as a scalable foundation for high-performance computing (HPC) and advanced physics research.

## ✨ Features

* **Robust Data Ingestion:** Features a custom, memory-safe pointer parser designed to extract and convert HMS/DMS coordinate systems from the 9,000+ record Yale Bright Star Catalog, cleanly bypassing standard `strtok` vulnerabilities.
* **Real-Time Planetarium Math:** Utilizes dynamic spherical trigonometry to translate Equatorial coordinates (Right Ascension/Declination) into Local Horizontal coordinates (Altitude/Azimuth) based on observer location and Sidereal Time.
* **Custom Graphics Pipeline:** Hardware-accelerated 3D rendering built from scratch using OpenGL 3.3 Core Profile, featuring high-DPI viewport scaling and magnitude-based point sizing.
* **Immersive Environment:** Alpha-blended, depth-masked transparent horizon planes and cardinal compass vectors provide true 3D spatial orientation without culling the southern hemisphere.

## 🛠️ Technology Stack

* **Core Engine:** Pure C
* **Graphics API:** OpenGL (3.3 Core Profile)
* **Windowing & Input:** GLFW (`glfw-x11`)
* **Extension Loader:** GLAD (Custom generated for Core Profile)
* **3D Mathematics:** `cglm` (Header-only integration for matrix operations)

## 🚀 Installation & Compilation (Linux)

This engine is designed for lightweight, low-level execution. The following build instructions are tailored specifically for Arch Linux environments.

### 1. Install Dependencies
```bash
sudo pacman -S glfw-x11 cglm
```
*(Note: The GLAD loader is included locally within the `include/` directory).*

### 2. Compilation
Compile the engine from source using `gcc`. 
> **Architectural Note:** We intentionally omit the `-lcglm` linker flag and rely entirely on the header-only implementation of `cglm`. This completely bypasses a known `glibc`/IFUNC dynamic linker segmentation fault prevalent on rolling-release distributions.

```bash
gcc main.c glad.c -I./include -o dark_architect -lglfw -lGL -lm
```

### 3. Execution
Ensure the `stars.csv` (Yale Bright Star Catalog) is located in the project's root directory.
```bash
./dark_architect
```

## 🎮 Controls
* **Arrow Keys:** Pan the 3D Camera (Pitch and Yaw)
* **ESC:** Terminate the simulation safely

## 🗺️ Project Roadmap

- [x] **Phase 1: The Headless Engine** - Dynamic memory architecture, structs, and robust CSV data extraction.
- [x] **Phase 2: The Planetarium Math** - Spherical trigonometry and Absolute-to-Relative coordinate translation.
- [x] **Phase 3: Real-Time Visualization** - OpenGL context generation, 3D Camera matrices, and environment shaders.
- [ ] **Phase 4: High-Performance Computing (HPC)** - Implementing a recursive Barnes-Hut Octree to transition from static rendering to $O(N \log N)$ mutual gravitational physics.
- [ ] **Phase 5: Research Extensions** - Modeling Self-Interacting Dark Matter (SIDM) and PySR (Symbolic Regression) integration.

---
**Author:** Arya Kukkadwal  
