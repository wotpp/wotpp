<p align=center><img alt="logo" src="docs/logo.png" width=30%></p>

# wot++
A small macro language for producing and manipulating strings.

![c++](https://img.shields.io/badge/c%2B%2B-%3E%3D17-blue.svg?style=flat)
[![license](https://img.shields.io/github/license/Jackojc/wotpp.svg?style=flat)](./LICENSE)
![code size](https://img.shields.io/github/languages/code-size/Jackojc/wotpp?style=flat-square)
[![issues](https://img.shields.io/github/issues/Jackojc/wotpp.svg?style=flat)](https://github.com/Jackojc/wotpp/issues)
[![discord](https://img.shields.io/discord/537732103765229590.svg?label=discord&style=flat)](https://discord.gg/RmgjcES)

### Prerequisites
- A C++17 compliant compiler. (GCC & Clang work)

### Build & Run

#### With make:
`make`
`./build/wpp <file>`

#### With meson:
```
$ meson builddir
$ ninja -C builddir
$ ninja -C builddir test # to run the tests
```

You can pass extra options to the meson command, or by running `meson configure -Doption1=value -Doption2=value ...` in the build directory.
For example, to enable stripping debug symbols on install, and set the build type to release, pass `-Dbuildtype=release -Dstrip=true`.

List of built-in options can be found [here](https://mesonbuild.com/Builtin-options.html).

### Installation
> Todo...

### License
This project uses the MPL-2.0 license. (check [LICENSE.md](LICENSE.md))

### Progress & Discussion
You can join the discord server in order to follow progress and/or contribute to discussion of the project. (https://discord.gg/RmgjcES)

