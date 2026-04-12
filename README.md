## New York Chronicles — Custom MTA:SA Client

Custom MTA:SA client build for [New York Chronicles](https://newyorkchronicles.online) Roleplay.

Based on [multitheftauto/mtasa-blue](https://github.com/multitheftauto/mtasa-blue/) with custom modifications for our server including streaming optimizations, model remapping, and client-side enhancements.

### Build

#### Requirements
- [Visual Studio 2022+](https://visualstudio.microsoft.com/vs/) with C++ desktop development
- [Microsoft DirectX SDK](https://wiki.multitheftauto.com/wiki/Compiling_MTASA#Microsoft_DirectX_SDK)

#### Steps
1. Copy `EncryptionKeys.example.h` to `Client/multiplayer_sa/EncryptionKeys.h` and fill in your keys
2. Run `win-create-projects.bat`
3. Open `Build/MTASA.sln`
4. Build

> `EncryptionKeys.h` is gitignored — you need your own keys to build a connecting client.

#### Linux (server only)
```sh
./linux-build.sh
```

### Links
- [Website](https://newyorkchronicles.online)
- [Forum](https://forum.newyorkchronicles.online)
- [Discord](https://discord.newyorkchronicles.online)

### License

Source code is licensed under GPLv3. See [LICENSE](./LICENSE).

Grand Theft Auto and all related trademarks are Rockstar North 1997-2026.
