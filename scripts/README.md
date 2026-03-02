## What does this script do
Converts a given jpeg image into NV12 data.

> [!NOTE]
> This script only works in Linux

## Preparing environment and installing dependencies
```
mkdir .python_env
pip -m venv .python_env
source .python_env/bin/activate
pip install Pillow
pip install numpy
```
## Running
```
python frame_format_conver.py picture1.jpg picture1.nv12 nv12
```
