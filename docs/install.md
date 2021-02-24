# Installation

> # signifies root
> $ signifies user

### Ubuntu 18.04
Enable use of PPAs:
```
# apt install software-properties-common
```

Install GCC-10:
```
# add-apt-repository ppa:ubuntu-toolchain-r/test
# apt-get update
# apt install g++-10
```

Install Meson:
```
$ pip3 install --user meson
$ export PATH=$PATH:~/.local/bin
```

Install Ninja and libreadline:
```
# apt install libreadline7 libreadline-dev ninja-build
```

Clone:
```
$ git clone https://github.com/Jackojc/wotpp && cd wotpp
```

Build:
```
$ CXX=g++-10 meson build
$ ninja -C build
```

Install:
```
$ cd build/
# meson install
```

### Void Linux
Install Meson, Ninja and libreadline:
```
# xbps-install -S meson ninja readline readline-devel
```

Clone:
```
$ git clone https://github.com/Jackojc/wotpp && cd wotpp
```

Build:
```
$ meson build
$ ninja -C build
```

Install:
```
$ cd build/
# meson install
```

