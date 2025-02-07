# OBS Shared-memory Ring Buffer Tools

## Introduction

This plugin is intended to provide tools to utilize the [shm_ringbuffers library](https://github.com/directrix1/shm_ringbuffers) as a conduit for simple interprocess video data sharing. This is currently enabled by the SRB Filter which outputs the video frames from the scene on which it is applied to the specified ringbuffer in RGBA format. This plugin only works on the platforms which **shm_ringbuffers** work on.

## Prerequisites

This plugin requires OBS development libraries, headers, dependencies to be installed in the usual locations and shm_ringbuffers to be installed in a place accessible by pkg-config (probably /usr) .

## Building

### Acquire the code

    git clone https://github.com/directrix1/obs-shmem-ringbuffer.git
    cd obs-shmem-ringbuffer

### Find your build preset

    cmake --list-presets

... let's assume we're gonna select 'linux-x86_64' ...

### Create build environment

    cmake --preset=linux-x86_64 --install-prefix=/usr

... in this case creates "build_x86_64/" folder ...

### Enter build environment and build

    cd build_x86_64
    ninja

### Install to system

    sudo ninja install
    
## Usage

The filter will be installed and is usable on indivual scenes. Add it to the scene, specify the hosted shared memory location and the ring buffer to send the video data to.

## License

This software is copyright (c) 2025 Edward Andrew Flick under GNU GPLv2 license.
To build 

## Additional Build Help

### Set Up Build Dependencies

Just like OBS Studio itself, plugins need to be built using dependencies available either via the `obs-deps` repository (Windows and macOS) or via a distribution's package system (Linux).

#### Choose An OBS Studio Version

By default the plugin template specifies the most current official OBS Studio version in the `buildspec.json` file, which makes most sense for plugins at the start of development. As far as updating the targeted OBS Studio version is concerned, a few things need to be considered:

* Plugins targeting _older_ versions of OBS Studio should _generally_ also work in newer versions, with the exception of breaking changes to specific APIs which would also be explicitly called out in release notes
* Plugins targeting the _latest_ version of OBS Studio might not work in older versions because the internal data structures used by `libobs` might not be compatible
* Users are encouraged to always update to the most recent version of OBS Studio available within a reasonable time after release - plugin authors have to choose for themselves if they'd rather keep up with OBS Studio releases or stay with an older version as their baseline (which might of course preclude the plugin from using functionality introduced in a newer version)

On Linux, the version used for development might be decided by the specific version available via a distribution's package management system, so OBS Studio compatibility for plugins might be determined by those versions instead.

#### Windows and macOS

Windows and macOS dependency downloads are configured in the `buildspec.json` file:

* `dependencies`:
    * `obs-studio`: Version of OBS Studio to build plugin with (needed for `libobs` and `obs-frontend-api`)
    * `prebuilt`: Prebuilt OBS Studio dependencies
    * `qt6`: Prebuilt version of Qt6 as used by OBS Studio
* `tools`: Contains additional build tools used by CI

The values should be kept in sync with OBS Studio releases and the `buildspec.json` file in use by the main project to ensure that the plugin is developed and built in sync with its target environment.

To update a dependency, change the `version` and associated `hashes` entries to match the new version. The used hash algorithm is `sha256`.

#### Linux

Linux dependencies need to be resolved using the package management tools appropriate for the local distribution. As an example, building on Ubuntu requires the following packages to be installed:

* Build System Dependencies:
    * `cmake`
    * `ninja-build`
    * `pkg-config`
* Build Dependencies:
    * `build-essential`
    * `libobs-dev`
* Qt6 Dependencies:
    * `qt6-base-dev`
    * `libqt6svg6-dev`
    * `qt6-base-private-dev`

## Build System Configuration

To create a build configuration, `cmake` needs to be installed on the system. The plugin template supports CMake presets using the `CMakePresets.json` file and ships with default presets:

* `macos`
    * Universal architecture (supports Intel-based CPUs as Apple Silicon)
    * Defaults to Qt version `6`
    * Defaults to macOS deployment target `11.0`
* `macos-ci`
    * Inherits from `macos`
    * Enables compile warnings as error
* `windows-x64`
    * Windows 64-bit architecture
    * Defaults to Qt version `6`
    * Defaults to Visual Studio 17 2022
    * Defaults to Windows SDK version `10.0.18363.657`
* `windows-ci-x64`
    * Inherits from `windows-x64`
    * Enables compile warnings as error
* `linux-x86_64`
    * Linux x86_64 architecture
    * Defaults to Qt version `6`
    * Defaults to Ninja as build tool
    * Defaults to `RelWithDebInfo` build configuration
* `linux-ci-x86_64`
    * Inherits from `linux-x86_64`
    * Enables compile warnings as error
* `linux-aarch64`
    * Provided as an experimental preview feature
    * Linux aarch64 (ARM64) architecture
    * Defaults to Qt version `6`
    * Defaults to Ninja as build tool
    * Defaults to `RelWithDebInfo` build configuration
* `linux-ci-aarch64`
    * Inherits from `linux-aarch64`
    * Enables compile warnings as error

Presets can be either specified on the command line (`cmake --preset <PRESET>`) or via the associated select field in the CMake Windows GUI. Only presets appropriate for the current build host are available for selection.
