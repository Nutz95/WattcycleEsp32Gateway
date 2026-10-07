Import("env")
import subprocess
from pathlib import Path

repo = Path(env["PROJECT_DIR"])
script = repo / "scripts" / "bundle_web.ps1"
print("Bundling web assets before filesystem image...")
subprocess.check_call(
    ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script)],
    cwd=str(repo),
)
