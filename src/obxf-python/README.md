# obxfpy

Python bindings for the [OB-Xf](https://github.com/surge-synthesizer/OB-Xf)
synthesizer. Exposes the full synthesis engine for offline rendering, DSP
prototyping, and patch exploration.

## Building

    pip install scikit-build-core pybind11
    pip install .

Or build in-place for development:

    pip install --no-build-isolation -e .

## Usage

```python
import obxfpy
import numpy as np


engine = obxfpy.ObxfEngine(44100.0)

def find_patch(engine, name):
    import os
    for base in (engine.get_factory_patches_path(), engine.get_user_patches_path()):
        for root, _, files in os.walk(base):
            for f in files:
                if f == name:
                    return os.path.join(root, f)
    return None

PATCH_NAME = "A Modern Day Warrior.fxp"

path = find_patch(engine, PATCH_NAME)

if path is None:
    print(f"Could not find patch '{PATCH_NAME}' in factory or user folders.")
    sys.exit(1)

engine.load_patch(path)
engine.note_on(60, 100)

L, R = engine.process(44100)
```