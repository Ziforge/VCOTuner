# VCOTuner

[![Build](https://github.com/Ziforge/VCOTuner/actions/workflows/build.yml/badge.svg)](https://github.com/Ziforge/VCOTuner/actions/workflows/build.yml)

A JUCE based tuner application for VCOs, VCFs and other analog gear. It runs on Windows, Mac and Linux.

This is a fork of [TheSlowGrowth/VCOTuner](https://github.com/TheSlowGrowth/VCOTuner) with:
- Updated JUCE to 8.0.4 for macOS Sequoia (15.x) compatibility
- Added CV output calibration feature for DC-coupled audio interfaces

## Overview

**How tuning usually works** - Tuning is usually a tedious ping-pong game between adjusting a fine tune pot and adjusting one or multiple tuning trimmers. Whenever a trimmer has been adjusted, the fine tune pot has to be adjusted as well to bring the pitch back to a specific note.

**How tuning works with the app** - The app spits out midi notes and measures the frequency. This is done for multiple notes in a user selectable note range. At first the note in the center of the range is selected as the reference pitch. All other measurements will be compared to this reference. This removes the need to adjust the fine tune pot. Tuning the oscillator is just a matter of tweaking the trimmers and looking at the screen. Takes no longer than a few minutes. See the video below.

**The application can also produce a report** that features measurements in the highest accuracy and over a very wide pitch range. Reports are saved as a *.png file including information on the device under test and the CV interface that was used.

## New: CV Calibration Feature

This fork adds CV output and calibration capabilities:

- **CV Output** - Generate precise voltages via DC-coupled audio interfaces (Expert Sleepers ES-8/ES-9, MOTU, etc.)
- **Automated Calibration** - Automated sweep: outputs CV, measures VCO frequency, builds correction tables
- **Voltage Standards** - Support for both 1V/Oct and Hz/V standards
- **Export Formats** - Export calibration data to CSV, JSON, and Ornament & Crime compatible formats

## Video Tutorial

This video shows how to use it:

<a href="http://www.youtube.com/watch?feature=player_embedded&v=JpMFTOBXuv8
" target="_blank"><img src="http://img.youtube.com/vi/JpMFTOBXuv8/0.jpg"
alt="Youtube tutorial video" width="400" border="0" /></a>

## Download

[Head over to the "release" section of this repository to download the latest release.](https://github.com/Ziforge/VCOTuner/releases/latest)

## Building from Source

### Prerequisites

- CMake 3.12 or later
- C++17 compatible compiler
- Platform-specific dependencies (see below)

### macOS

```bash
git clone --recursive https://github.com/Ziforge/VCOTuner.git
cd VCOTuner
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The built app will be in `build/VCOTuner_artefacts/Release/VCOTuner.app`

### Windows

```bash
git clone --recursive https://github.com/Ziforge/VCOTuner.git
cd VCOTuner
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Linux

Install dependencies first:

```bash
sudo apt-get install libasound2-dev libcurl4-openssl-dev libfreetype6-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxinerama-dev \
    libxrandr-dev libxrender-dev libwebkit2gtk-4.1-dev
```

Then build:

```bash
git clone --recursive https://github.com/Ziforge/VCOTuner.git
cd VCOTuner
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Help to improve it

[If you find bugs, please raise an issue here!](https://github.com/Ziforge/VCOTuner/issues)

## Credits

Original application by [TheSlowGrowth](https://github.com/TheSlowGrowth/VCOTuner)

## Are you on Muff's?

[Here's a thread on MuffWiggler. Post your tuning reports here, if you like](https://www.muffwiggler.com/forum/viewtopic.php?p=2276045)
