import os

target_path = "/usr/local/bin/dmgr"

content = """
~/.local/share/DriveMgr/bin/launcher/launcher
"""
with open(target_path, "w") as f:
    f.write(content)

os.chmod(target_path, 0o755)

print(f"Installed command at {target_path}")