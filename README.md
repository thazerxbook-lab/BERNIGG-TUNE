# BERNIGG TUNE — VST3 source project

A first functional prototype of a monophonic vocal pitch-correction effect for Windows/FL Studio, built with JUCE. The UI uses a blue/neon-green/violet UFO/alien theme inspired by the supplied artwork.

## Important limitations

This is an **MVP prototype**, not a polished commercial Auto-Tune replacement. It includes block-based autocorrelation pitch estimation, note quantization (chromatic/major/minor), adjustable correction response, a simple two-head delay-line pitch shifter, wet/dry mix, output gain, key selection and hard-correction toggle. The delay-line shifter can produce audible artifacts, especially on complex audio, and the pitch detector can misidentify notes on breathy, noisy, polyphonic or very low-level input. It does not preserve formants. Test on a duplicate vocal track before using it in a release.

The source has not been compiled or tested in FL Studio in this environment. Do not treat this package as a prebuilt `.vst3` binary.

## Requirements

- Windows 10/11, 64-bit
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+
- Git
- Internet access on first configure/build so CMake can download JUCE 8.0.6

## Build VST3

Open **x64 Native Tools Command Prompt for VS 2022**, then run from this folder:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BERNIGG_TUNE_VST3
```

Expected output location:

```text
build\BERNIGG_TUNE_artefacts\Release\VST3\BERNIGG TUNE.vst3
```

If the exact target name differs in your CMake generator, run `cmake --build build --config Release --target help` and select the VST3 target shown there.

## Install in FL Studio

1. Copy `BERNIGG TUNE.vst3` into `C:\Program Files\Common Files\VST3\` (copy the whole `.vst3` bundle/folder).
2. Open FL Studio and go to **Options → Manage plugins**.
3. Ensure the VST3 location is included, then run **Find installed plugins**.
4. Add **BERNIGG TUNE** to a mixer insert and monitor at a safe volume.
5. Set the vocal key and scale, then adjust Retune Speed and Mix. Hard correction enables faster, stronger correction.

## Parameters

- Retune Speed: 0–400 ms
- Mix: 0–100%
- Output: -18 to +12 dB
- Key: C through B
- Scale: Chromatic, Major, Minor
- Hard correction: faster/stronger correction response

## Troubleshooting

- If CMake cannot download JUCE, verify Git and internet access, then rerun the configure command.
- If Visual Studio components are missing, install the C++ desktop workload and Windows SDK.
- If FL Studio does not see the plugin, verify the bundle exists in the VST3 folder and rescan plugins.
- If the correction sounds rough, reduce Mix or disable Hard correction. This prototype is intended for experimentation and further DSP development.
