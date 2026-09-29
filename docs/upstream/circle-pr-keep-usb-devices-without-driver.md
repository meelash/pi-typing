<!--
Pull request for https://github.com/rsta2/circle
Base branch: develop    Compare branch: fix/keep-usb-devices-without-driver (in your fork)
Title: usb/usbdevice: Keep devices without supported function
Replace #ISSUE below with the issue number.
-->

Fixes #ISSUE.

A USB device without a supported function was deleted after
`CUSBDevice::Initialize()` returned `FALSE`. That freed its address number
while the device, which is not reset, kept answering at that address. The next
enumerated device could then get the same address. The port was also
enumerated again on every hub scan.

With this change such a device stays enumerated but unconfigured (after
`SET_CONFIGURATION 0`, as before). Its address stays allocated until it is
unplugged, and it is no longer re-enumerated.

- `CUSBDevice::Initialize()` sets a new `m_bIdle` flag and returns `TRUE`
  instead of `FALSE`.
- `CUSBDevice::Configure()` returns `TRUE` for an idle device without
  configuring anything.

**Tests**

- Build: `lib`, `lib/usb`, `lib/input`, `lib/fs` and `sample/08-usbkeyboard`
  for RPi 3, 4 and 5 in AArch64, and RPi 1, 2, 3 and 4 in AArch32 (Arm GNU
  Toolchain 15.2.Rel1). All build, with no warnings in `usbdevice.cpp`.
- QEMU `raspi3b` (AArch64), `sample/08-usbkeyboard` with `-append
  "usbignore=int3-0-0"`, a `usb-kbd` and a `usb-tablet`, then hot-plugging a
  second `usb-kbd` (the reproduction from the issue), on `develop` a2e72a72:
  - before: the tablet is found and removed on every scan, and the new
    keyboard is identified with the tablet's interface `int3-0-0` and removed;
  - after: the tablet is reported once and kept, and the new keyboard is
    found with `int3-1-1` and configured.
- Real hardware: Raspberry Pi 3 B+ (AArch64, based on Step51.1) with
  `EXCLUDE_USB_NET`. The on-board LAN7515 stays idle, and a Logitech Unifying
  receiver and a wired keyboard both work.
- Not tested on real hardware: RPi 4/5 (`CXHCIUSBDevice::Initialize()` also
  calls `CUSBDevice::Initialize()`, so an idle device is kept there too) and
  AArch32.
