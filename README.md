# LanguagePerformance
- 一个用于对比多种语言的性能差距的项目。项目包括 C++ SDL3, C++ SFML, C++ raylib, Rust Macroquad, Rust Bevy, Java Swing, Java JavaFX, Java LibGDX, Go Ebiten, Python Pygame, Python Pyglet, HTML5 多个语言实现<br>
- A project designed to compare the performance gap across multiple programming languages. This includes implementations across multiple languages and frameworks, such as C++ (SDL3, SFML, raylib), Rust (Macroquad, Bevy), Java (Swing, JavaFX, LibGDX), Go (Ebiten), Python (Pygame, Pyglet), and HTML5.

# How to use 如何使用
- 从 [GitHub Releases](https://github.com/LittleApple12CD/LanguagePerformance/releases) 下载发布包，解压ZIP文件，然后使用 run_benchmark.ps1 进行自动测试，也可以通过运行子目录的EXE程序或BAT脚本来手动测试
- Download the release package from [GitHub Releases](https://github.com/LittleApple12CD/LanguagePerformance/releases), extract the ZIP file, and use run_benchmark.ps1 for automated testing. Alternatively, you can run the EXE programs or BAT scripts in the subdirectories for manual testing.

# The Performance Tests 性能测试结果

**T1 - N-Body (CPU Compute)**

- **Go (Ebiten)** : 943.98 FPS
- **Rust (Bevy)** : 342.49 FPS
- **Java (LibGDX)** : 259.38 FPS
- **C++ (SFML 3.0)** : 236.73 FPS
- **Java (JavaFX)** : 227.37 FPS
- **C++ (raylib)** : 213.80 FPS
- **C++ (SDL3)** : 210.92 FPS
- **Rust (Macroquad)** : 203.88 FPS
- **Python (Pyglet + Numba)** : 163.21 FPS
- **HTML5 (WebGL)** : 159.44 FPS
- **Python (Pygame + Numba)** : 124.39 FPS
- **Java (Swing)** : 64.33 FPS

**T2 - 2D Squares (Draw Call Intensive)**

- **C++ (raylib)** : 3573.62 FPS
- **HTML5 (WebGL)** : 3361.34 FPS
- **Java (LibGDX)** : 1527.90 FPS
- **C++ (SDL3)** : 1124.48 FPS
- **Java (JavaFX)** : 1001.16 FPS
- **Rust (Macroquad)** : 996.34 FPS
- **Go (Ebiten)** : 952.75 FPS
- **C++ (SFML 3.0)** : 814.35 FPS
- **Python (Pyglet + Numba)** : 594.05 FPS
- **Rust (Bevy)** : 321.25 FPS
- **Python (Pygame + Numba)** : 177.45 FPS
- **Java (Swing)** : 64.55 FPS

**T3 - 3D Cubes (Vertex Compute)**

- **HTML5 (WebGL)** : 2895.05 FPS
- **C++ (raylib)** : 2699.17 FPS
- **C++ (SFML 3.0)** : 2142.53 FPS
- **Java (LibGDX)** : 1643.26 FPS
- **Java (JavaFX)** : 990.89 FPS
- **Rust (Macroquad)** : 989.11 FPS
- **Go (Ebiten)** : 645.00 FPS
- **Python (Pyglet + Numba)** : 634.13 FPS
- **C++ (SDL3)** : 507.00 FPS
- **Rust (Bevy)** : 458.46 FPS
- **Python (Pygame + Numba)** : 130.99 FPS
- **Java (Swing)** : 64.54 FPS

# Environment Dependencies 环境依赖

- **Java** : JDK 21, Maven, Gradle
- **C++** : MSYS2 UCRT64, CMake, Raylib, SFML, SDL3
- **Python** : Python 3.12, Pygame, Pyglet, Numpy, Numba
- **Rust** : Rustup 1.29, Cargo 1.97, Macroquad, Bevy
- **Go** : Go 1.21.5, Ebiten















