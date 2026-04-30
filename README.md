
# Project-FreeGen

**Project-FreeGen** is an open-source alternative to proprietary scaling solutions, designed specifically for the handheld gaming community (Steam Deck, Legion Go, Ally) and power users who want to know what’s actually happening under the hood.

I’m tired of "magic" proprietary code that offers zero transparency. Why should upscaling and frame generation be hidden away? This project is a **glass box**: modular, readable, and built in public so you can follow along with the wins (and my goofy mistakes) in real-time. ^-^

## <.< The Vision
Drawing heavy inspiration from the logic of **Magpie** and **lsfg_vk**, Project-FreeGen is a modular scaffold. I want the community to be able to plug in their own frame generation models or scaling algorithms as extensions down the line. Let's build something transparent. <.<

## [::] Architecture
```
┌─────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│  PipeWire/XDG   │────▶│  Capture Backend │────▶│ Vulkan Staging   │
│  Desktop Portal │     │  (MemFd/Mmap)    │     │  Buffer (CPU)     │
└─────────────────┘     └──────────────────┘     └────────┬─────────┘
                                                          │
                                                          ▼
┌─────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│  ImGui Overlay  │◀────│  Graphics Pipeline│◀────│ Compute Shader   │
│  (Settings UI)  │     │  (Quad + Sampler) │     │ (FSR/Passthrough)│
└─────────────────┘     └──────────────────┘     └────────┬─────────┘
                                                          │
                                                          ▼
                                                   ┌──────────────────┐
                                                   │ Upscaled Texture │
                                                   │ (GPU Storage)    │
                                                   └──────────────────┘
```

## 🛠️ The Lab & Workflow
* **The Rig:** Developed primarily on a **Zephyrus G14**, but built to run everywhere.
* **Vulkan Compute:** High-performance upscaling (FSR 1.0 & Integer Scaling) without melting your CPU.
* **FSR 3.1:** Currently in the early stages of a modular, readable implementation.
* **PipeWire/Portal:** Efficient, low-latency screen capture for Linux.
* **Dockerized Workflow:** The whole dev environment is tucked into Docker so we can mess with Vulkan headers without setting the Arch install on fire. >:/

## (O) Hardware Targets
* **Steam Deck (Arch/SteamOS):** The main target for that sweet, handheld optimization.
* **Legion Go / Z1 Extreme:** Windows port/fork testing is a high priority—I want this to be universal for the handheld crowd.

## \o/ Building & Running
All tasks are managed through `manage.sh`.

```bash
# Build the project
./manage.sh build

# Run the application
./manage.sh run

# Get a shell inside the dev container
./manage.sh shell
```

## 🤝 Build In Public
I’m currently leveraging local LLMs to keep the architecture clean while I'm deep in the weeds. Want to help? Here's the hit list:
- [ ] Wayland layer shell overlay (transparent, click-through)
- [ ] Frame generation (interpolation)
- [ ] DMA-BUF zero-copy buffer import
- [ ] Windows capture backend (DXGI Desktop Duplication)

#OpenSource #Vulkan #FSR3 #SteamDeck #BuildInPublic #LegionGo #ZephyrusG14
