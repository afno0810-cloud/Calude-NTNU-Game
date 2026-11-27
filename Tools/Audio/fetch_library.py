"""Lists and extracts files from the sound library zips (never the whole zip unless asked).

The library lives outside git: GS_SOUND_LIBRARY (default C:\\Users\\afno0\\Documents\\Lydbibliotek), one folder per
source, e.g. Kenney_ImpactSounds\\kenney_impact-sounds.zip. Extracted files go to <folder>\\extracted\\<path in zip>,
which is where prep_sounds.py reads them.

    python -I Tools/Audio/fetch_library.py --list Kenney_ImpactSounds [--grep footstep]
    python -I Tools/Audio/fetch_library.py --extract            (only the files named in sources.json)
    python -I Tools/Audio/fetch_library.py --extract-all Snake_AuthenticGunSounds

Files from the library are data: they are only read and copied, never run. Paths that try to leave the folder
(absolute or with "..") are refused.
"""
import argparse
import json
import os
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
LIBRARY = os.environ.get("GS_SOUND_LIBRARY", r"C:\Users\afno0\Documents\Lydbibliotek")


def safe(name):
    return not (name.startswith(("/", "\\")) or ":" in name or ".." in name.replace("\\", "/").split("/"))


def zips(folder):
    root = os.path.join(LIBRARY, folder)
    if not os.path.isdir(root):
        return []
    return [os.path.join(root, f) for f in sorted(os.listdir(root)) if f.lower().endswith(".zip")]


def do_list(folder, grep):
    for z in zips(folder):
        with zipfile.ZipFile(z) as zf:
            for info in zf.infolist():
                if info.is_dir():
                    continue
                if grep and grep.lower() not in info.filename.lower():
                    continue
                print(f"{folder}/{info.filename}\t{info.file_size / 1e6:.2f} MB")


def extract(folder, wanted=None):
    out_root = os.path.join(LIBRARY, folder, "extracted")
    done = 0
    for z in zips(folder):
        with zipfile.ZipFile(z) as zf:
            for info in zf.infolist():
                if info.is_dir() or not safe(info.filename):
                    continue
                if wanted is not None and info.filename not in wanted:
                    continue
                dst = os.path.join(out_root, *info.filename.split("/"))
                if os.path.exists(dst) and os.path.getsize(dst) == info.file_size:
                    continue
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                with zf.open(info) as src, open(dst, "wb") as f:
                    f.write(src.read())
                done += 1
    print(f"{folder}: {done} file(s) extracted")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", metavar="FOLDER")
    ap.add_argument("--grep")
    ap.add_argument("--extract", action="store_true")
    ap.add_argument("--extract-all", metavar="FOLDER")
    a = ap.parse_args()
    if a.list:
        do_list(a.list, a.grep)
    elif a.extract_all:
        extract(a.extract_all)
    elif a.extract:
        with open(os.path.join(HERE, "sources.json"), encoding="utf-8") as f:
            entries = json.load(f)["sounds"]
        per_folder = {}
        for e in entries:
            for s in [e] + e.get("layers", []):
                src = s.get("src", "")
                if "/" in src and not src.startswith("gen:"):
                    folder, inner = src.split("/", 1)
                    per_folder.setdefault(folder, set()).add(inner)
        for folder, wanted in sorted(per_folder.items()):
            extract(folder, wanted)
    else:
        ap.print_help()


if __name__ == "__main__":
    main()
