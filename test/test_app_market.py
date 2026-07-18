import json
import tempfile
import unittest
from pathlib import Path

from scripts.app_market import (
    ManifestError,
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

    def test_missing_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            template = Path(directory) / "template.json"
            template.write_text(json.dumps(self.template("missing.bin")), encoding="utf-8")
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
            template_data = self.template(str(artifact))
            template_data["releaseAssets"] = [{"asset": "logo.png", "source": str(icon)}]
            template = root / "template.json"
            template.write_text(json.dumps(template_data), encoding="utf-8")
            assets = root / "release"
            manifest_path = assets / "app-market.json"
            manifest = generate(template, manifest_path, assets, "v1.2.0")
            self.assertEqual(manifest["release"]["partitions"][0]["offset"], "0x0000")
            self.assertEqual(manifest["release"]["version"], "1.2.0")
            self.assertEqual(set(manifest), {"schemaVersion", "app", "release"})
            self.assertTrue((assets / "app-firmware.bin").is_file())
            validate(manifest_path, assets)

    @staticmethod
    def template(source):
        return {
            "schemaVersion": 1,
            "app": {
                "id": "super-wifi-duck",
                "name": "SuperWifiDuck",
                "description": "Test",
                "supportedDevices": ["super-wifi-duck"],
                "tags": ["wifi", "security"],
                "icon": "logo.png",
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
