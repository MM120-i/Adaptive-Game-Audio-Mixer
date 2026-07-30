# Building

## Prerequisites

- **Visual Studio 2026** with the **Desktop development with C++** workload
- **CMake 3.22+** (bundled with VS 2026, or install separately)
- **Windows 10 SDK** (included with VS)

## Quick start

```bash
git clone https://github.com/MM120-i/Adaptive-Game-Audio-Mixer.git
cd Adaptive-Game-Audio-Mixer
git submodule update --init
echo "SPOTIFY_CLIENT_ID=your_client_id_here" > .env
.\build.bat
```

## Build targets

| Command | Description |
|---------|-------------|
| `.\build.bat` | Build app + tests (Debug) |
| `.\build.bat Debug` | Build specific config |
| `.\build.bat Release` | Build optimized Release config |
| `.\build.bat test` | Build + run 60 unit tests |
| `.\build.bat lint` | Run MSVC `/analyze` + cppcheck |
| `.\build.bat run` | Launch the app (Debug) |
| `.\build.bat clean` | Delete `build/` directory |
| `.\build.bat release` | Build Release + create `AudioMixer-Release.zip` |

## Where things go

```
build/
└── AudioMixer_artefacts/
    └── Debug/          # Debug build output
        ├── AudioMixer.exe
        └── auth/       # OAuth callback pages
```

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_ANALYZE` | `OFF` | Enable MSVC `/analyze` warnings |

```bash
cmake -S . -B build -DENABLE_ANALYZE=ON
```

## GitHub Actions

CI runs on push/PR:
- Builds Debug + runs all 60 tests
- MSVC `/analyze` for static analysis
- `cppcheck` for deeper linting

Configured in `.github/workflows/`.
