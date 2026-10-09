Import("env")

"""PlatformIO pre-script: embed secrets/ecoflow/login_key.bin."""

import subprocess
from pathlib import Path

repo = Path(env["PROJECT_DIR"])
script = repo / "scripts" / "embed_ecoflow_login_key.py"
subprocess.check_call([env["PYTHONEXE"], str(script)], cwd=str(repo))
