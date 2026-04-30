# Project-FreeGen >:3

**Project-FreeGen** is an open-source alternative to proprietary scaling solutions, designed specifically for the handheld gaming community (Steam Deck, Legion Go, Ally) and Windows/Linux power users.

I’m tired of "magic" proprietary code. This project aims to be a **glass box**: modular, readable, and community-driven. I want people to be able to follow along with the "pros" (and my goofy mistakes) in real-time. ^-^

## <.< The Vision
The goal is to provide a free, open-source alternative to tools like Lossless Scaling. Drawing heavy inspiration from the logic of **Magpie** and **lsfg_vk**, Project-FreeGen is built to be a modular scaffold[cite: 2].

I want the community to be able to plug in their own frame generation models or scaling algorithms as extensions down the line. Let's build something transparent. <.<[cite: 2]

## [::] Features & Tech
- **Real-time game capture** via PipeWire and XDG Desktop Portal[cite: 2].
- **GPU-accelerated upscaling** using Vulkan compute shaders[cite: 2]:
  - **FSR 1.0** - AMD's FidelityFX Super Resolution[cite: 2].
  - **Integer scaling** - Nearest-neighbor pixel-perfect scaling[cite: 2].
- **FSR 3.1 Integration** - Currently in the early stages of modular implementation[cite: 2].
- **Dear ImGui overlay** for runtime settings (TAB to toggle)[cite: 2].
- **Dockerized Workflow** - Dev workflow tucked into Docker so you can mess with Vulkan headers without making a mess of your system[cite: 2].

## (O) Architecture
