"""Resident body loader via public FILEHANDLE, entered through NC/RUN."""
from pathlib import Path
import subprocess,sys
subprocess.run([sys.executable,str(Path(__file__).resolve().parents[1]/'emu80/check-run-body.py')],check=True)
