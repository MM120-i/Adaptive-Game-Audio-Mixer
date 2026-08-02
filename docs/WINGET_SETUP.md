# WinGet Distribution

AudioMixer is distributed through the Windows Package Manager (WinGet), the package
manager preinstalled on Windows 10/11. Once the package is accepted into the
community repository, users install it with:

```
winget install MM120i.AudioMixer
```

The `winget-releaser` GitHub Action (`.github/workflows/winget-release.yml`) opens a
WinGet manifest PR automatically every time a release is published.

## How it works

1. You publish a GitHub Release with the `AudioMixer-Release.zip` asset
   (built via `build.bat release`).
2. The `Publish to WinGet` workflow runs the `winget-releaser` action, which
   uses your `WINGET_TOKEN` to push new manifests to a fork of
   `microsoft/winget-pkgs` and opens a PR.
3. WinGet community bots validate the manifest (checksums, URLs, metadata) and
   merge it. `winget install MM120i.AudioMixer` works for all users shortly after.

## Prerequisites

- The package must have **at least one version already accepted** in the
  community repository before the Action will work, the Action creates
  _update_ manifests based on the previous one. Submit the first version
  manually (see below).
- Your release zip must contain `AudioMixer.exe` **plus the `auth\` folder**
  (the app reads its OAuth callback pages from disk at runtime) and nothing
  else, no `.pdb`. `build.bat release` does this automatically.

## One-time setup (do this once)

### 1. Create a classic Personal Access Token

1. GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic).
2. **Generate new token (classic)**.
3. Scope: tick **`public_repo`** only.
4. Copy the token value immediately (shown once).

> The action does **not** support fine-grained PATs, it must be a classic token.

### 2. Add the token as a repo secret

1. Repo → Settings → Secrets and variables → Actions.
2. **New repository secret**.
3. Name: `WINGET_TOKEN` — value: the token.

### 3. Fork the WinGet community repository

Fork `microsoft/winget-pkgs` under the same account that owns this repo
(`MM120-i`). The Action pushes manifest updates to this fork and opens the PR
from it. (If you fork under a different account, add
`fork-user: <username>` to the Action inputs in `winget-release.yml`.)

### 4. Submit the first version manually

The official tool (`wingetcreate new`) **cannot parse AudioMixer's exe** because
it ships no version resource, the tool bails with "Failed to parse the
package". Instead, write the three manifests by hand (see
`docs/winGet/MM120i.AudioMixer/` for the working copies used for `1.0.0`):

```
manifests/m/MM120i/AudioMixer/1.0.0/
├── MM120i.AudioMixer.yaml                   # version manifest
├── MM120i.AudioMixer.installer.yaml         # installer manifest (portable zip)
└── MM120i.AudioMixer.locale.en-US.yaml      # defaultLocale manifest
```

Key values:

- `PackageIdentifier: MM120i.AudioMixer`
- `InstallerType: portable`
- `Architecture: x64`
- `InstallerUrl`: the GitHub Release zip URL
- `InstallerSha256`: SHA256 of the zip (`Get-FileHash AudioMixer-Release.zip -Algorithm SHA256`)

Create a branch in your fork of `winget-pkgs`, push the three files under
`manifests/m/MM120i/AudioMixer/<version>/`, and open a PR to
`microsoft/winget-pkgs`. Sign the Microsoft CLA when the bot asks: community
bots validate and merge within a few days.

> The zip contains a folder (`auth\`), which is fine — portable packages may
> ship supporting files as long as the portable exe is at the zip root.

## Releasing a new version (after setup)

1. `build.bat release` → `AudioMixer-Release.zip`
2. Create a GitHub Release with tag `vN.N.N` (no `v` prefix issue, the Action
   strips it automatically) and attach the zip.
3. The Action files the WinGet manifest PR. Nothing else to do.

## Troubleshooting

| Problem                                  | Fix                                                                                                                                                  |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| Action fails: "identifier ... not found" | First version not merged yet — complete step 4 first.                                                                                                |
| Action can't find the asset              | `installers-regex` is `\.zip$`; confirm the release asset is named `AudioMixer-Release.zip` and the release is **published** (not a draft).          |
| PR validation fails on hash              | Rebuilt zip changed content — `build.bat release` is deterministic; re-run and re-upload, or let the Action recompute (it always recomputes hashes). |
| "Login denied" / push failed             | Token is expired/revoked or lacks `public_repo` scope — regenerate and update the secret.                                                            |
