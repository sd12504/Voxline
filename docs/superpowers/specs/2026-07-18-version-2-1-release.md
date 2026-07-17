# VOXLINE 2.1 Release Versioning

## Goal

Release the completed VOXLINE build as version `2.1.0`, presented to users as `2.1`.

## Version surfaces

- The CMake project version is `2.1.0`; this is the source for plug-in bundle metadata.
- The editor footer presents `VOXLINE 2.1`.
- GitHub Actions uses `2.1.0` for Windows VST3 archives, macOS PKG/DMG files, artifact names, and release attachments.

## Compatibility

- This is a release-version update only. It does not change parameter IDs, state schema, DSP, preset compatibility, or plug-in formats.

## Verification

- Confirm no shipped version surface still identifies the release as `2.0.0`.
- Build and run the existing test suite locally.
- Re-run the existing GitHub Actions matrix and verify its Windows VST3 package is named `VOXLINE-2.1.0-Windows-x64-VST3.zip`.
