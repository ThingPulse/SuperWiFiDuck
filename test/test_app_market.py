import json
import tempfile
import unittest
from pathlib import Path

from scripts.app_market import (
    APPLICATION_ID,
    ManifestError,
    SUPPORTED_DEVICE_IDS,
    generate,
    is_prerelease,
    normalize_version,
    sha256_file,
    validate,
    validate_ranges,
)


class AppMarketTests(unittest.TestCase):
    def test_version_normalization(self):
        self.assertEqual(normalize_version("refs/tags/v1.2.3-rc.1"), "1.2.3-rc.1")
        self.assertEqual(normalize_version("1.2.3"), "1.2.3")
        with self.assertRaises(ManifestError):
            normalize_version("release-1.2")

    def test_prerelease_detection(self):
        self.assertTrue(is_prerelease("v1.2.0-alpha.1"))
        self.assertTrue(is_prerelease("1.2.0-beta.1"))
        self.assertTrue(is_prerelease("1.2.0-rc.1"))
        self.assertFalse(is_prerelease("v1.2.0"))

    def test_sha256_generation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "firmware.bin"
            path.write_bytes(b"abc")
            self.assertEqual(
                sha256_file(path),
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            )

    def test_missing_firmware_fails_release_preparation(self):
        with tempfile.TemporaryDirectory() as directory:
            template = Path(directory) / "template.json"
            template.write_text(
                json.dumps(self.template("missing.bin", Path(directory) / "logo.png")),
                encoding="utf-8",
            )
            with self.assertRaises(ManifestError):
                generate(template, Path(directory) / "manifest.json", None, "1.0.0")

    def test_overlapping_ranges(self):
        partitions = [
            {"name": "first", "asset": "one.bin", "offset": "0x1000"},
            {"name": "second", "asset": "two.bin", "offset": "0x1010"},
        ]
        with self.assertRaises(ManifestError):
            validate_ranges(partitions, {"one.bin": 32, "two.bin": 16})

    def test_generate_discover_offsets_and_structure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            artifact = root / "firmware.bin"
            artifact.write_bytes(b"firmware")
            icon = root / "logo.png"
            icon.write_bytes(b"png")
            template_data = self.template(str(artifact), str(icon))
            template = root / "template.json"
            template.write_text(json.dumps(template_data), encoding="utf-8")
            assets = root / "release"
            manifest_path = assets / "app-market.json"
            manifest = generate(template, manifest_path, assets, "v1.2.0-rc.3")
            expected_app_keys = {"id", "name", "description", "supportedDevices", "tags", "icon"}
            expected_icon_keys = {"asset", "sha256"}
            expected_partition_keys = {"name", "asset", "offset", "sha256"}
            self.assertEqual(manifest["app"]["id"], APPLICATION_ID)
            self.assertEqual(manifest["app"]["supportedDevices"], SUPPORTED_DEVICE_IDS)
            self.assertEqual(manifest["app"]["icon"]["asset"], "logo.png")
            self.assertEqual(manifest["app"]["icon"]["sha256"], sha256_file(assets / "logo.png"))
            self.assertEqual(
                manifest["release"]["partitions"][0]["sha256"],
                sha256_file(assets / "app-firmware.bin"),
            )
            self.assertEqual(manifest["release"]["partitions"][0]["offset"], "0x0000")
            self.assertEqual(manifest["release"]["version"], "1.2.0-rc.3")
            self.assertEqual(set(manifest), {"schemaVersion", "app", "release"})
            self.assertEqual(set(manifest["app"]), expected_app_keys)
            self.assertEqual(set(manifest["app"]["icon"]), expected_icon_keys)
            self.assertEqual(set(manifest["release"]), {"version", "partitions"})
            self.assertEqual(set(manifest["release"]["partitions"][0]), expected_partition_keys)
            self.assertTrue((assets / "app-firmware.bin").is_file())
            validate(manifest_path, assets, "v1.2.0-rc.3")

    def test_checksum_mismatch_fails_release_preparation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            artifact = root / "firmware.bin"
            artifact.write_bytes(b"firmware")
            icon = root / "source-logo.png"
            icon.write_bytes(b"png")
            template = root / "template.json"
            template.write_text(json.dumps(self.template(str(artifact), str(icon))), encoding="utf-8")
            assets = root / "release"
            manifest_path = assets / "app-market.json"
            generate(template, manifest_path, assets, "v1.2.0-rc.3")

            (assets / "logo.png").write_bytes(b"modified")
            with self.assertRaisesRegex(ManifestError, "SHA-256 mismatch for logo.png"):
                validate(manifest_path, assets, "v1.2.0-rc.3")

            (assets / "logo.png").write_bytes(b"png")
            (assets / "app-firmware.bin").write_bytes(b"modified")
            with self.assertRaisesRegex(ManifestError, "SHA-256 mismatch for app-firmware.bin"):
                validate(manifest_path, assets, "v1.2.0-rc.3")

    def test_missing_staged_asset_fails_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            artifact = root / "firmware.bin"
            artifact.write_bytes(b"firmware")
            icon = root / "source-logo.png"
            icon.write_bytes(b"png")
            template = root / "template.json"
            template.write_text(json.dumps(self.template(str(artifact), str(icon))), encoding="utf-8")
            assets = root / "release"
            manifest_path = assets / "app-market.json"
            generate(template, manifest_path, assets, "v1.2.0-rc.3")
            (assets / "logo.png").unlink()
            with self.assertRaises(ManifestError):
                validate(manifest_path, assets, "v1.2.0-rc.3")

    def test_wrong_identifiers_and_string_icon_fail_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            artifact = root / "firmware.bin"
            artifact.write_bytes(b"firmware")
            icon = root / "source-logo.png"
            icon.write_bytes(b"png")
            template = root / "template.json"
            template.write_text(json.dumps(self.template(str(artifact), str(icon))), encoding="utf-8")
            assets = root / "release"
            manifest_path = assets / "app-market.json"
            manifest = generate(template, manifest_path, assets, "v1.2.0-rc.3")

            for field, invalid in (
                ("id", "super-wifi-duck"),
                ("supportedDevices", ["super-wifi-duck"]),
                ("icon", "logo.png"),
            ):
                broken = json.loads(json.dumps(manifest))
                broken["app"][field] = invalid
                manifest_path.write_text(json.dumps(broken), encoding="utf-8")
                with self.assertRaises(ManifestError):
                    validate(manifest_path, assets, "v1.2.0-rc.3")

    def test_manifest_version_must_match_tag(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            artifact = root / "firmware.bin"
            artifact.write_bytes(b"firmware")
            icon = root / "source-logo.png"
            icon.write_bytes(b"png")
            template = root / "template.json"
            template.write_text(json.dumps(self.template(str(artifact), str(icon))), encoding="utf-8")
            assets = root / "release"
            manifest_path = assets / "app-market.json"
            generate(template, manifest_path, assets, "v1.2.0-rc.3")
            with self.assertRaisesRegex(ManifestError, "does not match tag"):
                validate(manifest_path, assets, "v1.2.0-rc.4")

    @staticmethod
    def template(source, icon_source):
        return {
            "schemaVersion": 1,
            "app": {
                "id": APPLICATION_ID,
                "name": "SuperWifiDuck",
                "description": "Test",
                "supportedDevices": SUPPORTED_DEVICE_IDS,
                "tags": ["wifi", "security"],
                "icon": {"asset": "logo.png", "source": str(icon_source)},
            },
            "release": {
                "partitions": [
                    {
                        "name": "firmware",
                        "asset": "app-firmware.bin",
                        "offset": "0x0000",
                        "source": source,
                    }
                ]
            },
        }


if __name__ == "__main__":
    unittest.main()
