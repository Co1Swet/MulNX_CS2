# 🚨 MulNX Safety Notice

**Please read and fully understand the following before use. By using this software, you are deemed to accept all risks and agree to the applicable license terms you have chosen (AGPL v3 or MulNX 2.0).**

## Background Knowledge

- **-insecure**: This launch option is an optional parameter provided by Valve for CS2. It marks your client as untrusted, preventing you from joining VAC‑protected servers as a player, in exchange for the ability to load any third‑party plugins without being monitored by VAC.
- **GOTV**: Provided by Valve, GOTV allows an untrusted client to connect to any match server — even protected ones — as an **out‑of‑game observer (not an in‑game player)** , receiving game data for spectating.
- **GSI**: Short for *Game State Integration*, GSI is a network‑based system from Valve that transmits real‑time match data from a locally connected spectator client (whether in‑game or out‑of‑game) and can be used for HUD development. (This project **does not** make use of GSI internally, but allows compatible use of GSI alongside this project.)

## Core Safety Warnings

- **Only Official Launch Method**: You MUST launch the game via CS2Injector.exe and click the “Launch CS2” button. This automatically appends the `-insecure` flag to disable VAC, protecting your account.
- **Manual Injection is Strictly Prohibited**: Any form of manual DLL injection is forbidden, as it may trigger VAC and result in a permanent ban.
- **Close Third‑Party Platforms**: While running the software, close all third‑party gaming platforms to avoid the risk of false bans. Third‑party platforms may not recognize the `-insecure` status.

## Inherent Risks

- Any third‑party program carries a potential risk of being falsely flagged by anti‑cheat systems.
- Game updates may cause the software to become temporarily unavailable or unstable.

## Special Notes

- The DLL component does **not** enforce an `-insecure` check by itself, but this does **not** mean you are allowed to use it in live matches.
- This design exists solely for the convenience of using the software in your **own environment**, **not** for **game cheating**.
- If you attempt to use it for **cheating**, the extensive **runtime characteristics** of the MulNX framework are more than enough for **any** anti‑cheat system to **easily** identify and ***ban*** your **illegal use**!
