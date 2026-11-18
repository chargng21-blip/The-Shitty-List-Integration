# TSL Integration — The Shitty List inside Geometry Dash

A Geode **5.11.2 / Geometry Dash 2.2081 Windows** mod source project. A themed popup appears via the **TSL** button on the Geometry Dash main menu.

## Features

- Two tabs: **THE SHITTY LIST** and **THE SHITTY LIST PLUS**.
- TSL-inspired dark midnight-blue/navy design with blue and violet accent lines.
- Rank, level name, creator, and click-to-inspect details.
- Once the live lists load, **rank badges on GD level-info screens** for exactly matching level names (TSL and/or TSL+).
- Real-time text search (name, rank, or creator).
- Six cards per page with previous/next controls.
- **REFRESH**: asynchronously requests the authoritative lists at `https://tsl.pages.dev/list/tsl` and `https://tsl.pages.dev/list/tslplus` and parses their current HTML.
- **WEB**: opens the matching website in your browser.
- If the website fails or its markup changes, shows a deliberately limited ten-entry offline sample with a visible warning. **Offline samples are not claimed to be up to date**.

## Build the Windows `.geode` mod

### Easiest: GitHub Actions

1. Create a GitHub repository and **upload the contents of this folder**, including the hidden `.github/workflows/build-windows.yml` file.
2. Open the repo's **Actions** tab and run **Build TSL Integration (Windows)** via **Run workflow**.
3. When it completes successfully, open that run, download the **TSL-Integration-Windows** artifact ZIP, and extract the resulting `.geode` file.
4. Copy the `.geode` file into your Geometry Dash **`geode/mods`** directory. Restart the game and use the **TSL** button on the main menu.

### Local Geode SDK

Install the Windows Geode SDK following the Geode mod developer docs. With `GEODE_SDK` set and CMake/Ninja/Clang configured:

```powershell
cmake -S . -B build -G Ninja -DGEODE_TARGET_PLATFORM=Win64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Look for a `.geode` file inside `build/`.

## Caveats / implementation status

- **This ZIP contains source code, not a compiled `.geode`.** A Geode SDK/game runtime was not available to compile/test it in the authoring environment. It includes a CI workflow but a successful Geode build has **not yet been verified**.
- Geode's game UI and API change over versions. The source targets the listed Geode/GD versions and may need minor API adjustments at compile time.
- Rankings are scraped from site HTML, not an official stable API. If the website switches to client-side rendering or changes the card format, live loading may stop working; you will still see a clearly labeled offline sample in the browser. **The rank badges only use successfully loaded live rankings** to avoid presenting potentially stale sample positions as authoritative.
- Rank badges match by exact level name (case-insensitive), not by GD level ID; duplicate name matches may be ambiguous.
- This is an **independent, unofficial** fan-made integration and is **not** affiliated with The Shitty List or Integrated Demonlist's original developers. It does not reuse the supplied native mod's DLL or binary.

## UI preview

Double-click **`preview.html`** to see the dark-theme list browser concept in any browser. It is an interactive **design mockup**, not a playable Geode mod; the preview intentionally uses only ten offline sample entries per tab.

## Parser tests (no Geode SDK needed)

```bash
g++ -std=c++20 -Wall -Wextra -Werror -O2 src/ListParser.cpp tests/parser_test.cpp -o parser_test
./parser_test
```

## UI sources

- [The Shitty List](https://tsl.pages.dev/list/tsl)
- [The Shitty List Plus](https://tsl.pages.dev/list/tslplus)
