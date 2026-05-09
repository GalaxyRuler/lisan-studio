# License Notes

Lisan Studio private beta stages third-party runtime components.

## Application

The application source license is not finalized for public distribution. Private
beta artifacts must include this notice and release notes.

## Qt

Qt is dynamically linked and deployed with `windeployqt6`. Before public
distribution, confirm whether the release uses Qt LGPL obligations or a
commercial Qt license.

The private beta package includes the MSYS2 `qt6-base` license payload under
`licenses\qt6-base`.

## Python

The package script copies a local Python runtime into `runtime\python`. Include
Python license files from the staged runtime before distributing outside private
testing.

The private beta package includes `licenses\Python-LICENSE.txt`.

## Lughat Althuban

`lughat-althuban` is installed into the staged runtime from the local source
tree passed to `scripts\package.ps1`.

The private beta package includes `licenses\lughat-althuban-LICENSE`.
