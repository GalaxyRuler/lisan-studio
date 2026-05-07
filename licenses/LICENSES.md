# License Notes

Arabic Code Studio Qt private beta stages third-party runtime components.

## Application

The application source license is not finalized for public distribution. Private
beta artifacts must include this notice and release notes.

## Qt

Qt is dynamically linked and deployed with `windeployqt6`. Before public
distribution, confirm whether the release uses Qt LGPL obligations or a
commercial Qt license.

## Python

The package script copies a local Python runtime into `runtime\python`. Include
Python license files from the staged runtime before distributing outside private
testing.

## Lughat Althuban

`lughat-althuban` is installed into the staged runtime from the local source
tree passed to `scripts\package.ps1`.

