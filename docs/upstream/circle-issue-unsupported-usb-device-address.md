<!--
Draft issue for https://github.com/rsta2/circle/issues/new
Title: USB device without a driver keeps its address after being deleted, so the next device gets a duplicate address (RPi 1-3)
Attach: qemu-sample08-before.png, qemu-sample08-after.png (in this folder),
        and the Pi 3 B+ diagnostic photos (optional).
-->

**Description of the bug**

When a USB device has no supported function, `CUSBDevice::Initialize()` sends
`SET_CONFIGURATION 0` and returns `FALSE`. The hub then deletes the
`CUSBDevice`, and its destructor returns the device address to
`s_DeviceAddressPool`. The physical device is not reset or disabled at that
point, so it keeps answering at that address. The next device that is
enumerated can be given the same address number, and from then on two devices
answer at one address.

In addition, the port of the deleted device is enumerated again on every hub
scan, so the device repeatedly goes through found, "no supported function",
removed. Each pass frees another address that is still in use.

Symptoms seen on real hardware (Raspberry Pi 3 B+). Our application
excludes the network drivers (`EXCLUDE_USB_NET`), so the on-board LAN7515
Ethernet (`ven424-7800`) has no driver and goes through this cycle every
~0.4 s:

- A full-speed Logitech Unifying receiver (`ven46d-c52b`) plugged in later
  fails on the first request to its new address:
  ```
  dwhci: Transaction failed (status 0x82)
  usbdbg: ctl adr3 spd1 mps32 req 80 06 0200 0000 len9 -> FAIL res-1 err1
  usbdev0-1: Cannot get configuration descriptor (short)
  usbdev0-1: Device ven46d-c52b removed
  ```
  (`usbdbg` is extra logging I added in `CUSBHostController::ControlMessage()`.
  The device descriptor is read fine at address 0; the failure starts right
  after `SET_ADDRESS`.) At boot it sometimes works, depending on timing.
- A low-speed wired keyboard behind the internal hub failed the same way at
  high speed. With `usbspeed=full` it works, presumably because the
  full-speed Ethernet ignores low-speed packets.

**Steps to reproduce (QEMU, unmodified sample)**

This reproduces with `sample/08-usbkeyboard`, using `usbignore` to create a
device without a driver:

1. Build Circle and `sample/08-usbkeyboard` for `-r 3 --qemu` (AArch64).
2. Run:
   ```
   qemu-system-aarch64 -M raspi3b -kernel kernel8.img -append "usbignore=int3-0-0" \
     -device usb-kbd,id=k1 -device usb-tablet,id=t1 \
     -monitor unix:mon.sock,server,nowait
   ```
3. When "Just type something!" appears, hot-plug a second keyboard in the
   monitor: `device_add usb-kbd,id=k2`.

Result (screenshot `qemu-sample08-before.png`): the tablet (ignored) is found
and removed again on every scan. The hot-plugged device is identified as
"QEMU USB Keyboard", but its configuration shows the *tablet's* interface
`int3-0-0`, so it is ignored and removed too:

```
00:00:11.65 usbdev0-1: Device ven627-1 found (FS)
00:00:11.68 usbdev0-1: Product: QEMU QEMU USB Tablet
...
00:00:11.79 usbdev0-1: Device ven627-1 removed
00:00:11.96 usbdev0-1: Device ven627-1 found (FS)
00:00:11.99 usbdev0-1: Product: QEMU QEMU USB Keyboard
00:00:12.04 usbdev0-1: Interface int3-0-0 found
00:00:12.04 ufactory: Ignoring device/interface int3-0-0
00:00:12.04 usbdev0-1: Function is not supported
00:00:12.04 usbdev0-1: Device has no supported function
00:00:12.10 usbdev0-1: Device ven627-1 removed
```

Expected: the second keyboard is found with interface `int3-1-1` and works.

**Proposed fix**

Keep a device without a supported function enumerated (unconfigured) instead
of deleting it, the way Linux leaves a device without a bound driver. Its
address stays allocated until the device is actually unplugged, and the port
is no longer re-enumerated on every scan. With this change the same QEMU test
gives the expected result (`qemu-sample08-after.png`): the tablet is reported
once and kept ("Port 2: Device configured"), and the hot-plugged keyboard is
found as `int3-1-1`. It also fixed the Logitech receiver on the Pi 3 B+.

```diff
--- a/include/circle/usb/usbdevice.h
+++ b/include/circle/usb/usbdevice.h
@@ -123,6 +123,8 @@ private:
 
 	CUSBConfigurationParser *m_pConfigParser;
 
+	boolean m_bIdle;	// enumerated, but no driver: kept to hold its address
+
 	CUSBFunction *m_pFunction[USBDEV_MAX_FUNCTIONS];
 
--- a/lib/usb/usbdevice.cpp
+++ b/lib/usb/usbdevice.cpp
@@ (both constructors)
-	m_pConfigParser (0)
+	m_pConfigParser (0),
+	m_bIdle (FALSE)
@@ CUSBDevice::Initialize (void)
 			LogWrite (LogWarning, "Cannot reset configuration");
 		}
 
-		return FALSE;
+		// Keep the device (unconfigured) instead of deleting it. Deleting it
+		// would free its address number while the device still answers at that
+		// address, so the next device could get the same address and both would
+		// reply at once. It would also be re-enumerated on every hub scan.
+		m_bIdle = TRUE;
+
+		return TRUE;
 	}
@@ CUSBDevice::Configure (void)
 		return FALSE;
 	}
 
+	if (m_bIdle)			// no driver: nothing to configure
+	{
+		return TRUE;
+	}
+
 	boolean bResult = FALSE;
```

The patch applies cleanly to `develop` (616eb68). An alternative would be to
disable or reset the hub port before the address is freed when a device is
dropped. I'm happy to change it to whatever approach you prefer and send a
pull request against `develop`.

**Affected version(s) and hardware**

- Circle Step51.1 (the code is unchanged on current `master` and `develop`).
- Raspberry Pi 3 B+, AArch64. Also reproduced in QEMU `raspi3b` (10.2).
- Not tested: 32-bit builds, and Raspberry Pi 4/5.
  `CXHCIUSBDevice::Initialize()` calls `CUSBDevice::Initialize()`, so the
  change affects the xHCI path too. The address collision itself probably
  cannot happen there, because xHCI assigns addresses per slot, but the
  device would now be kept instead of re-enumerated.

**Log output or screenshots**

- `qemu-sample08-before.png`: unmodified Circle, output above.
- `qemu-sample08-after.png`: with the patch.
- Pi 3 B+ photos of the on-screen log (optional).

**Side note (separate issue?)**

While testing in QEMU, unplugging an active keyboard with
`device_del` triggers `dwhcidevice.cpp(1142): assertion failed: pStageData != 0`.
This happens with and without the patch above. Unplugging on the real Pi 3 B+
did not show it, so it may be QEMU-specific.
