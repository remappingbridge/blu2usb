# Build and toolchain lock

BLU2USB-G01 fixes the reproducible build inputs used by CI. Changes to these values require an explicit gate/contract update rather than an incidental dependency bump.

## Canonical CI environment

The machine-readable lock is `ci/toolchain.env`:

- runner: `ubuntu-24.04`;
- Raspberry Pi Pico SDK: `2.2.0` exact Git tag;
- `gcc-arm-none-eabi` Debian/Ubuntu package: `15:13.2.rel1-2`;
- upstream ARM GCC release represented by that package: `13.2.Rel1`.

CI installs the compiler package at the exact package version and verifies it with `dpkg-query`. The Pico SDK is cloned at the exact `2.2.0` tag and checked with `git describe --tags --exact-match`.

Support packages such as newlib/libstdc++ are supplied by the same Ubuntu 24.04 archive as the pinned compiler. The compiler package and SDK are the canonical versioned toolchain inputs for this gate.

## Host build

```sh
cmake -S . -B build-host \
  -DBLU2USB_BUILD_PICO=OFF \
  -DBLU2USB_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

The host build executes the bootstrap contract test and architecture enforcement test.

## Pico 2 W production scaffold

With Pico SDK 2.2.0 available at `PICO_SDK_PATH`:

```sh
cmake -S . -B build-pico \
  -DBLU2USB_BUILD_PICO=ON \
  -DBLU2USB_BUILD_TESTS=OFF \
  -DPICO_BOARD=pico2_w \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-pico --parallel
```

G01 deliberately emits only the production `blu2usb_picow.uf2` scaffold. It has no product Bluetooth/USB/UI behavior yet and no diagnostic CDC/UART/debug firmware variant.
