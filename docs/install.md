# Installing SendBloom

SendBloom ships as an Audio Unit and a VST3 for macOS 11 or later, on Apple
silicon and Intel. Both are universal binaries.

## Download

Get the disk image and `SHA256SUMS.txt` from the
[Releases page](https://github.com/Niko96-dotcom/SendBloom/releases).

## Verify before you install

Put both files in the same folder and run:

```bash
shasum -a 256 -c SHA256SUMS.txt
```

Every line must say `OK`. If any line says `FAILED`, delete the download and
fetch it again — do not install it.

macOS will also verify the notarization ticket itself when you open the disk
image. If you want to check that explicitly:

```bash
xcrun stapler validate SendBloom-<version>-macOS.dmg
spctl --assess --type open --context context:primary-signature -v SendBloom-<version>-macOS.dmg
```

For a full accounting of what is in the release — hashes, signing identity,
notarization submission, source commit — see `release-manifest.json` and
`provenance.json`, attached to the same release. `SendBloom-<version>-sbom.json`
lists third-party dependencies and their licences.

## Install

Open the disk image and follow `INSTALL.txt`. Which of the two forms you get
depends on the release:

**Installer package.** Open `SendBloom-<version>.pkg` and follow the prompts.
It installs both plug-ins and registers receipts so upgrading and uninstalling
are clean. You will be asked for an administrator password, because the system
plug-in folders are not user-writable.

**Plug-in bundles.** Copy them into place:

| From the disk image | To |
|---|---|
| `VST3/SendBloom.vst3` | `/Library/Audio/Plug-Ins/VST3/` |
| `Components/SendBloom.component` | `/Library/Audio/Plug-Ins/Components/` |

```bash
sudo ditto "/Volumes/SendBloom <version>/VST3/SendBloom.vst3" \
           "/Library/Audio/Plug-Ins/VST3/SendBloom.vst3"
sudo ditto "/Volumes/SendBloom <version>/Components/SendBloom.component" \
           "/Library/Audio/Plug-Ins/Components/SendBloom.component"
```

Restart your DAW afterwards. Hosts scan plug-ins at launch and will not notice
a new install in a running session.

## Confirm what you installed

Ask the bundle directly rather than trusting a host's cached plug-in list:

```bash
plutil -extract CFBundleShortVersionString raw -o - \
  /Library/Audio/Plug-Ins/VST3/SendBloom.vst3/Contents/Info.plist
```

For the Audio Unit, the authoritative check is whether the AU layer accepts it:

```bash
AU_TYPE="$(plutil -extract 'AudioComponents.0.type' raw -o - \
  /Library/Audio/Plug-Ins/Components/SendBloom.component/Contents/Info.plist)"
auval -v "$AU_TYPE" SbLm NkMo
```

SendBloom currently reports `aumf` because it accepts MIDI; the command reads
the installed component's type instead of assuming `aufx`.

If you installed from the package, the receipts also record the version:

```bash
pkgutil --pkg-info com.nikoaudiolabs.sendbloom.vst3
pkgutil --pkg-info com.nikoaudiolabs.sendbloom.au
```

## Upgrading

Install the new release over the old one; both forms replace the existing
bundles in place. Then quit and reopen your DAW, and confirm the version with
the commands above — a host that was running during the install will still be
showing you the previous build.

Do not keep equal-version SendBloom VST3 bundles in both the system and
user-local plug-in folders. They share the stable class ID required for session
compatibility, so an equal version gives the host no reliable ordering signal.
The release tooling checks this before an installation and requires the new
candidate to be strictly newer; the DAW must still confirm which binary it
actually instantiated afterwards.

If you have the earlier `1.0.0-rc0` candidate installed from a manual copy, it
left no receipts. Remove it as described below before installing a release, so
there is no ambiguity about which build a host loaded.

Presets and saved plug-in state carry across releases. State written by old
betas that contained the removed `authentic_color` parameter is accepted and
that value is discarded.

## Uninstalling

```bash
sudo rm -rf /Library/Audio/Plug-Ins/VST3/SendBloom.vst3
sudo rm -rf /Library/Audio/Plug-Ins/Components/SendBloom.component
```

If you installed from the package, also clear the receipts:

```bash
sudo pkgutil --forget com.nikoaudiolabs.sendbloom.vst3
sudo pkgutil --forget com.nikoaudiolabs.sendbloom.au
```

SendBloom installs no helpers, launch agents, system extensions, drivers or
background processes. Those two bundles and the two receipts are everything it
puts on your machine.

Your own presets and host session data are not touched by any of the above.

## Troubleshooting

**The plug-in does not appear in my DAW.**
Restart the DAW. If it still does not appear, check the bundle is where you
expect it with the `plutil` command above, and confirm your host scans the
system plug-in folders rather than a user folder.

**macOS says the disk image cannot be opened / is from an unidentified developer.**
Check you downloaded a real release asset and that `shasum -a 256 -c` passes.
A genuine release is notarized and stapled and opens without a warning. Do not
work around a Gatekeeper refusal by removing quarantine attributes — a refusal
means the file is not what it should be.

**Logic does not list the Audio Unit.**
Logic caches AU scan results. Run the `AU_TYPE`/`auval` command above first; if
that passes, the component is installed correctly and the problem is Logic's cache
— reset the AU cache from Logic's plug-in manager and relaunch.

**I upgraded but the version has not changed.**
The host is almost certainly still running the old copy from memory. Quit it
completely, then re-check with `plutil` before anything else.

**Which build am I actually running?**
Compare the installed bundle against the release manifest:

```bash
shasum -a 256 /Library/Audio/Plug-Ins/VST3/SendBloom.vst3/Contents/MacOS/SendBloom
```

and check the version and commit recorded in `provenance.json` for that
release.
