"""Совместимый вход автономного стенда source-apps/copybench."""
from pathlib import Path
import runpy

if __name__ == "__main__":
    runpy.run_path(str(Path(__file__).resolve().parents[2]/"source-apps/copybench/benchmark.py"), run_name="__main__")
