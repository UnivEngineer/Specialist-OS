"""Whole-file ASM wrappers over the real banked FILEHANDLE service."""
from pathlib import Path
import subprocess,sys
subprocess.run([sys.executable,str(Path(__file__).resolve().parents[1]/'emu80/check-unified-file-api.py')],check=True)
