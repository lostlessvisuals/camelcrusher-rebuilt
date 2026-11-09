import pathlib
import subprocess
import tempfile
import unittest


SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "packaging/macos/postinstall-vst2.sh.in"


class InstallerAliases(unittest.TestCase):
    def test_replaces_duplicates_with_recoverable_backup(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory).resolve()
            target = root / "system/CamelCrusher.vst"
            target.mkdir(parents=True)
            link = root / "user/CamelCrusher.vst"
            link.mkdir(parents=True)
            (link / "original").write_text("preserve me")
            backups = root / "outside scan paths/Backups"

            def install():
                subprocess.run([
                    "/bin/bash", "-c", 'source "$1"; ensure_symlink "$2" "$3" "$4"',
                    "test", str(SCRIPT), str(target), str(link), str(backups)
                ], check=True)

            install()
            self.assertTrue(link.is_symlink())
            self.assertEqual(link.resolve(), target)
            originals = list(backups.glob("*/CamelCrusher.vst/original"))
            self.assertEqual(len(originals), 1)
            self.assertEqual(originals[0].read_text(), "preserve me")
            install()
            self.assertEqual(len(list(backups.iterdir())), 1)
            link.unlink()
            link.symlink_to(root / "missing old bundle")
            install()
            self.assertEqual(link.resolve(), target)
            self.assertEqual(len(list(backups.iterdir())), 1)


if __name__ == "__main__":
    unittest.main()
