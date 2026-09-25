#!/bin/bash
# Regenerates the website's screenshots (docs/img/) with the desktop simulator.
# Runs the simulator tests (build/shots/) and the extra scenes in scripts/site/,
# then converts the screens the site uses to WebP.
set -euo pipefail
cd "$(dirname "$0")/.."

tests/sim/run.sh >/dev/null
out=build/site-shots
rm -rf "$out" && mkdir -p "$out"
run() {  # run <script> [seed progress file]: each scene starts from its own save folder
	local save
	save=$(mktemp -d)
	[ -z "${2:-}" ] || cp "$2" "$save/progress.txt"
	./build/sim/sim "$1" "$out" "$save" >/dev/null
	rm -rf "$save"
}
run scripts/site/no-keyboard.txt
run scripts/site/capitals.txt scripts/site/capitals-progress.txt
run scripts/site/combo-game.txt
run scripts/site/rank-up.txt
run scripts/site/players.txt build/shots/progress.txt

# site name = simulator screenshot
python3 - <<'EOF'
from PIL import Image
import os
shots = {
    "splash": "build/shots/01-splash", "players": "build/site-shots/players",
    "new-player": "build/shots/03-newprofile", "new-player-ur": "build/shots/41-newprofile-ur",
    "courses": "build/shots/04-courses", "map-start": "build/shots/05-map",
    "map": "build/site-shots/map-progress", "intro": "build/shots/06-intro",
    "intro-capitals": "build/site-shots/intro-capitals", "typing": "build/shots/07-typing",
    "combo": "build/site-shots/combo", "shift-tip": "build/site-shots/shift-tip",
    "capslock": "build/site-shots/capslock", "pause": "build/shots/28-pause",
    "results": "build/shots/08-results", "rank-up": "build/site-shots/rank-up", "results-practise": "build/shots/29-results-fail",
    "badges": "build/shots/27-badges", "game-intro": "build/shots/26b-gameintro-startlevel",
    "game": "build/site-shots/game-balloons", "game-over": "build/shots/26-gameover",
    "game-words-ur": "build/shots/55-gameintro-ur-words",
    "map-ar": "build/shots/10-map-ar", "intro-ar": "build/shots/11-intro-ar",
    "typing-ar": "build/shots/12-typing-ar", "courses-ur": "build/shots/42-courses",
    "map-ur": "build/shots/43-map-ur", "typing-ur": "build/shots/45-typing-ur-1",
    "typing-ur-review": "build/shots/48-typing-ur-18", "typing-ur-final": "build/shots/51-typing-ur-final",
    "results-ur": "build/shots/53-results-ur-final", "map-ur-done": "build/shots/54-map-ur-done",
    "badges-ur": "build/shots/58-badges-ur", "delete-player": "build/shots/22-delete",
    "log": "build/shots/31-diagnostics", "plug-keyboard": "build/site-shots/plug-keyboard",
    "no-keyboard-log": "build/site-shots/no-keyboard-log",
}
os.makedirs("docs/img", exist_ok=True)
for name, src in shots.items():
    Image.open(src + ".png").convert("RGB").save(f"docs/img/{name}.webp", quality=88, method=6)
print(f"{len(shots)} screenshots in docs/img/")
EOF
