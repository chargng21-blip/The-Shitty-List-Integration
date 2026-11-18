# TSL Integration v0.2.0 — The Shitty List in Geometry Dash

A Windows Geode mod for Geometry Dash **2.2081 / Geode 5.11.2**, styled after TSL's blue and violet site. This is an **unofficial** community project.

## What's new in 0.2.0

- **New location:** Open **Create > Search** in Geometry Dash. The `TSL` button is on the **level search screen**, like the inspiration mod Integrated Demonlist. Removed the button from the game's main menu.
- **Playable browsing:** Clicking a listed row now leaves the TSL popup and opens **Geometry Dash's native online level results** for that level. Click the matching result to see its level details and play it. If a trustworthy game ID is available, it searches by ID; otherwise, it searches the level's exact display name. This is not automatic direct PlayLayer launch.
- **UI repairs:** Rebuilt a 442×286 dialog with five clearly separated clickable rows/page, a compact footer, text that fits, better tab highlights, and blue/purple styling.
- **TSL and TSL+ tabs**, live search, page controls, website link, reload.
- Existing ranking markers on level-info screens retained; these appear **only after fresh list data** has been loaded.

## Update the existing mod on GitHub

1. Download the updated source ZIP and extract it.
2. Open your existing GitHub repository: `https://github.com/chargng21-blip/The-Shitty-List-Integration`.
3. Upload the updated files **into the same repository**, preserving their relative paths: `src/main.cpp`, `src/ListParser.hpp`, `src/ListParser.cpp`, `mod.json`, `README.md`, `preview.html`, `tests/parser_test.cpp` and `CMakeLists.txt`. You do **not** need to reupload `.github` if the existing workflow still works.
4. Commit the changes. A new **Build TSL Integration (Windows)** run should start automatically (the included workflow triggers on commits to the branch).
5. From the finished green Actions run, download its artifact ZIP, extract the `.geode` file, and **replace** the old `folk.tsl_integration.geode` in `Geometry Dash/geode/mods`. Restart the game.

If GitHub upload won't replace a file, open that file in your repo, click its pencil icon and replace the contents with the contents of the same local updated file, then commit. Repeat for changed files.

## Important limitations

- This is a **source project**. Its non-Geode parser tests have passed, but **v0.2.0 has not been compiled or tested inside Geometry Dash** here; GitHub Actions and in-game testing must confirm compatibility.
- The public list pages display names and creators but don't reliably supply verified GD level IDs in their readable text. Until IDs can be retrieved from a trusted endpoint, list rows will **open a native GD search by name**. If multiple results share a name, pick the one matching the TSL entry and creator.
- The site can change layout, in which case web parsing may stop working. The mod displays a short clearly labeled **offline sample** in that situation rather than falsely presenting it as live.
- Both `https://tsl.pages.dev/list/tsl` and `https://tsl.pages.dev/list/tslplus` are used, with no hard-coded claim that sample ranks are up to date.
- Rank badges match level names when no verified GD ID is available, so identically named levels can be ambiguous.

## Run parser tests locally (optional)

```bash
g++ -std=c++20 -Wall -Wextra -Werror -O2 src/ListParser.cpp tests/parser_test.cpp -o parser_test
./parser_test
```

## Credits and links

- [The Shitty List](https://tsl.pages.dev/list/tsl) and [The Shitty List Plus](https://tsl.pages.dev/list/tslplus), for the rankings and UI styling reference.
- [Integrated Demonlist](https://github.com/hiimjasmine00/IntegratedDemonlist), for the level-search placement and in-game browsing concept. No proprietary binary or assets are included.
