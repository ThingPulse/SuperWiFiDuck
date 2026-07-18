# Release process

GitHub Actions builds and publishes SuperWifiDuck releases for the ESP App Market. A release contains:

- `app-market.json`
- `app-firmware.bin`
- `logo.png`

The merged `app-firmware.bin` image is flashed at offset `0x0000` and contains the bootloader, partition table, and application.

## Prepare the release

1. Update the application version in `src/config.h`. The stable base version must match the tag, excluding a prerelease suffix:

   ```cpp
   #define VERSION "1.2.0"
   ```

2. Build and verify the release locally:

   ```bash
   pio run -e thingpulse-pendrive-s3
   python3 scripts/app_market.py generate --output dist/app-market.json --assets-dir dist
   python3 scripts/app_market.py validate --manifest dist/app-market.json --assets-dir dist
   python3 -m unittest test.test_app_market
   ```

3. Review, commit, and push the release changes:

   ```bash
   git status
   git add app-market.json app-market.template.json scripts test \
     .github/workflows/release.yml README.md RELEASE.md \
     post_extra_script.py .gitignore
   git commit -m "Prepare SuperWifiDuck release"
   git push origin HEAD
   ```

The tag must point to the commit containing the release workflow and all intended firmware changes.

## Test without publishing

Open **GitHub → Actions → Build and release firmware → Run workflow**. A manually dispatched run builds and validates the release files, then uploads them as workflow artifacts. It does not create a GitHub Release.

## Create a snapshot prerelease

Create and push a SemVer prerelease tag:

```bash
git tag -a v1.2.0-rc.1 -m "SuperWifiDuck 1.2.0-rc.1"
git push origin v1.2.0-rc.1
```

Tags containing SemVer prerelease suffixes such as `-alpha.1`, `-beta.1`, or `-rc.1` create GitHub prereleases. The ESP App Market imports these as snapshots.

Check the corresponding GitHub Actions run and verify that the GitHub prerelease contains all three expected assets before testing it through the snapshot catalog.

## Create a stable release

After the prerelease has been tested, create and push the stable tag:

```bash
git tag -a v1.2.0 -m "SuperWifiDuck 1.2.0"
git push origin v1.2.0
```

A tag without a prerelease suffix creates a normal GitHub Release. The ESP App Market imports normal releases into its stable catalog.

## Required repository setting

The release workflow uses the standard `GITHUB_TOKEN`; no additional secret is required. In the GitHub repository, open **Settings → Actions → General → Workflow permissions** and enable **Read and write permissions** so the workflow can create releases and upload assets.

## Correct an erroneous prerelease

Prefer publishing a new prerelease number, such as `v1.2.0-rc.2`, if the erroneous release may already have been imported or downloaded.

If the original prerelease must be removed, confirm the tag name carefully and then delete the GitHub release, remote tag, and local tag:

```bash
gh release delete v1.2.0-rc.1 --yes
git push origin :refs/tags/v1.2.0-rc.1
git tag -d v1.2.0-rc.1
```

Correct and commit the release contents before creating a replacement tag. Do not reuse a stable version that may already have been consumed; publish a new patch version instead.
