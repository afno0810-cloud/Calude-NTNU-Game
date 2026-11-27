"""Reads the Grafikk.Foto log lines and the screenshots of one photo run and writes a table (Markdown).

    python Tools/Unreal/foto_stats.py <log> <bildemappe> <etikett> <ut.md>

Per picture: mean brightness of the final image (0-255, after tone mapping), share of burnt-out pixels (all channels
>= 250) and of black pixels (luma < 12). The section "## <etikett>" in <ut.md> is replaced (other sections stay).
"""
import os
import re
import sys

import numpy as np
from PIL import Image

LINE = re.compile(r"Grafikk\.Foto: (\w) (kl\d\d)(\S*) frame ([\d.]+)/([\d.]+) game ([\d.]+) draw ([\d.]+) gpu ([\d.]+)/([\d.]+) ms \((\d+) bilder, (.*)\)")


def stats(path):
    a = np.asarray(Image.open(path).convert("RGB")).astype(np.float32)
    luma = 0.2126 * a[..., 0] + 0.7152 * a[..., 1] + 0.0722 * a[..., 2]
    burnt = np.all(a >= 250, axis=-1).mean() * 100
    black = (luma < 12).mean() * 100
    return luma.mean(), burnt, black


def main():
    log, folder, label, out = sys.argv[1:5]
    rows = []
    with open(log, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = LINE.search(line)
            if m:
                rows.append(m.groups())
    lines = [f"## {label}", "",
             "| Utsnitt | Tid | Bildetid snitt/maks | Spill | Tegning | GPU snitt/maks | Lysstyrke | Utbrent % | Svart % | Hva |",
             "|---|---|---|---|---|---|---|---|---|---|"]
    for (v, kl, suf, fr, frmax, game, draw, gpu, gpumax, n, name) in rows:
        img = os.path.join(folder, f"{v}_{kl}{suf}.png")
        if os.path.exists(img):
            mean, burnt, black = stats(img)
            st = f"{mean:.0f} | {burnt:.1f} | {black:.1f}"
        else:
            st = "– | – | –"
        lines.append(f"| {v}{suf} | {kl} | {fr}/{frmax} ms | {game} | {draw} | {gpu}/{gpumax} ms | {st} | {name} |")
    if not rows:
        lines.append("(ingen Grafikk.Foto-linjer i loggen)")
    lines.append("")
    section = "\n".join(lines)
    old = open(out, encoding="utf-8").read() if os.path.exists(out) else "# Runde 5a: målinger fra Grafikk.Foto\n\n"
    pat = re.compile(r"^## " + re.escape(label) + r"\n.*?(?=^## |\Z)", re.S | re.M)
    new = pat.sub(section + "\n", old) if pat.search(old) else old.rstrip("\n") + "\n\n" + section + "\n"
    with open(out, "w", encoding="utf-8") as f:
        f.write(new)
    print(section)


if __name__ == "__main__":
    main()
