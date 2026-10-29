# PlatformIO Setup

## Build and Flash
1. Open the \irmware\ folder in VS Code.
2. Click the checkmark (Build) on the bottom panel to compile.
3. Click the arrow (Upload) to flash the board.

## Troubleshooting (No space left on device)
If the build fails with 'No space left on device', move PlatformIO core to another drive by running this in PowerShell:
\[Environment]::SetEnvironmentVariable("PLATFORMIO_CORE_DIR", "D:\pio-core", "User")\
Then restart VS Code.
