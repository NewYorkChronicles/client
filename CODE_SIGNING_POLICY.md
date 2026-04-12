# Code Signing Policy

## Purpose

All official release binaries of the New York Chronicles client and launcher are digitally signed to ensure authenticity and integrity. This policy describes how code signing is managed for the project.

## Signed Artifacts

The following artifacts are signed before distribution:

- **Client executables** (`.exe`) — the modified MTA:SA client binaries
- **Launcher executable** (`Launcher.exe`) — the game launcher
- **Installer** (`NYCSetup.exe`) — the Inno Setup installer package

## Signing Process

1. Release builds are produced from the `main` or versioned branch (e.g. `1.6`)
2. Binaries are submitted for signing through SignPath
3. Signed artifacts are uploaded to the CDN for distribution
4. Only organization administrators can trigger signing

## Key Management

- Private signing keys are managed by the certificate authority (SignPath)
- Keys are never stored in the repository or on developer machines
- Signing is performed in a controlled CI/signing environment

## Verification

Users can verify the digital signature of any official binary by:

1. Right-clicking the `.exe` file in Windows Explorer
2. Selecting **Properties** → **Digital Signatures** tab
3. Confirming the signer is **New York Chronicles** or **izcarti**

## Unauthorized Builds

Binaries built from source without the signing certificate will not carry a valid signature. Users should only run officially signed binaries downloaded from [newyorkchronicles.online](https://newyorkchronicles.online).

## Contact

For questions about code signing, contact the project maintainer via [Discord](https://discord.newyorkchronicles.online).
