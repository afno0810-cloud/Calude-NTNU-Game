"""Decode compressed audio (OGG/MP3/FLAC) to 16-bit WAV with Blender's built-in audio library (aud).

Python on this PC has no OGG decoder, but Blender ships one. prep_sounds.py runs this for every OGG it needs:

    blender.exe -b --factory-startup --python Tools/Audio/blender_decode.py -- <list.txt>

<list.txt> has one "input|output" pair per line. The input is only read as audio data.
"""
import sys

import aud  # Blender's audio module


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not argv:
        print("blender_decode: no list given")
        return
    with open(argv[0], encoding="utf-8") as f:
        pairs = [line.rstrip("\n").split("|") for line in f if "|" in line]
    ok = 0
    for src, dst in pairs:
        try:
            snd = aud.Sound(src)
            snd.write(dst, rate=48000, channels=aud.CHANNELS_STEREO, format=aud.FORMAT_S16,
                      container=aud.CONTAINER_WAV, codec=aud.CODEC_PCM)
            ok += 1
        except Exception as e:  # keep going: one bad file must not stop the rest
            print(f"blender_decode: {src}: {e}")
    print(f"blender_decode: {ok}/{len(pairs)} decoded")


main()
