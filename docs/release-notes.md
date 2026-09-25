Typing Adventure {{VERSION}}: a touch-typing tutor for children, in English, Arabic and Urdu, for the **Raspberry Pi 3 B+**. It runs with no operating system and has no network, Wi-Fi or Bluetooth support.

Built from commit {{COMMIT}}.

## New SD card

1. Download **`typing-adventure-{{VERSION}}.img.xz`**.
2. Flash it to a microSD card with [Raspberry Pi Imager](https://www.raspberrypi.com/software/): *Choose OS → Use custom*, pick the file, then choose your card. Imager can open the compressed `.img.xz` file directly. Any card of 64 MB or more works; flashing erases it.
3. Put the card in the Pi 3 B+. Connect an HDMI screen, a USB keyboard and headphones or speakers, then power on.

Alternatively, format a card as FAT32 and copy the files from **`typing-adventure-{{VERSION}}-sdcard-files.zip`** onto it.

## Update an existing card (keeps players and progress)

1. Put the card in a computer.
2. Download **`kernel8.img`** and copy it onto the card, replacing the file of the same name.

Don't change the other files: `cmdline.txt` holds your settings, and `progress.txt` holds the players' progress.

## Settings

`cmdline.txt` on the card holds all settings on **one line**:

- Sound: `sounddev=sndpwm` for the headphone jack (default), `sounddev=sndhdmi` for speakers in the HDMI screen, `sounddev=none` for silence.
- Keep `usbspeed=full`; some keyboards aren't recognised without it.

## Using it

- **Arrows** move, **Enter** chooses, **Esc** goes back or pauses a lesson.
- **F2** turns sound on or off. **Delete** on the players screen removes a player (confirm with **Y**).
- Several keyboards can be plugged in at once.

## If something doesn't work

- **"Please plug in a keyboard" stays on screen:** after about 6 seconds a diagnostic log appears. Take a photo of it.
- **Press F12** on any screen to open the full log. Arrows, PgUp/PgDn and Home/End scroll it.
- **`kernel8-debug.img`** logs extra USB detail. Copy it onto the card and rename it to `kernel8.img`. To go back, copy the normal `kernel8.img` over it.

## Files

| File | What it is |
|---|---|
| `typing-adventure-{{VERSION}}.img.xz` | Complete SD card image |
| `typing-adventure-{{VERSION}}-sdcard-files.zip` | The same files, to copy onto a FAT32 card |
| `kernel8.img` | The program only, for updating a card |
| `kernel8-debug.img` | The program with extra USB logging, for troubleshooting |
| `SHA256SUMS.txt` | Checksums of the files above |
