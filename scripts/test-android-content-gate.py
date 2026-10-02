"""Keep the pinned PadMint audit before both Android CI uploads."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
AUDITOR = "f0efbb738cac934559e986143343dc8720a6fb1d"


class AndroidContentGateTests(unittest.TestCase):
    def job(self, name):
        workflow = (ROOT / ".github/workflows/ci.yml").read_text()
        match = re.search(r"^  " + re.escape(name) + r":\n(.*?)(?=^  \S|\Z)",
                          workflow, re.MULTILINE | re.DOTALL)
        self.assertIsNotNone(match)
        return match.group(1)

    def check_upload(self, name, path, build_command):
        job = self.job(name)
        command = 'python3 -B -m padmint audit "$GITHUB_WORKSPACE/' + path + '"'
        audit = job.index("- name: Audit Android contents before upload")
        upload = job.index("uses: actions/upload-artifact@")
        self.assertLess(job.index(build_command), audit)
        self.assertLess(job.index(command), upload)
        gate = job[audit:upload]
        upload_step = job[upload:].split("\n      - ", 1)[0]
        for bypass in ("continue-on-error", "if:", "||", "always()"):
            self.assertNotIn(bypass, gate)
            self.assertNotIn(bypass, upload_step)
        self.assertRegex(upload_step, re.compile(
            r"^          path: " + re.escape(path) + r"$", re.MULTILINE))
        self.assertIn("working-directory: .ci-padmint", gate)
        self.assertRegex(job[:audit], re.compile(
            r"repository: chrissotraidis/padmint\s+ref: " + AUDITOR
            + r"\s+path: \.ci-padmint\s+persist-credentials: false"))
        self.assertIn('python-version: "3.11"', job[:audit])
        self.assertNotIn("continue-on-error", job[:audit])

    def test_native_libraries_are_checked_before_upload(self):
        self.check_upload(
            "android-libs",
            "src/openrct2-android/app/build/intermediates/cmake/release/obj/${{ matrix.arch }}",
            "./gradlew app:externalNativeBuildRelease")

    def test_complete_apk_upload_directory_is_checked(self):
        self.check_upload("android", "artifacts", "./gradlew app:assembleRelease")
        self.assertIn("path: artifacts\n", self.job("android"))
        self.assertIn("- name: Setup keystore", self.job("android"))
        self.assertIn("- name: Upload gradle issues", self.job("android"))


if __name__ == "__main__":
    unittest.main()
