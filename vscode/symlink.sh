#!/bin/bash
# Sets up the Aurum VS Code extension by symlinking the in-repo
# tools/vscode-aurum folder into VS Code's extensions directory.
# Run this once after cloning the repo (or again if the symlink ever breaks).

set -e  # exit immediately if any command fails

# resolve the absolute path to this script's directory, so the command
# works no matter where you run it from
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TARGET_DIR="$HOME/.vscode/extensions/aurum-lang"

# sanity check: make sure the extension folder actually exists in-repo
# before trying to link it
if [ ! -d "$SOURCE_DIR" ]; then
   echo "Error: '$SOURCE_DIR' not found. Are you running this from the repo root?"
   exit 1
fi

# if a symlink (or folder) already exists at the target, remove it first
# so re-running this script is safe and idempotent
if [ -e "$TARGET_DIR" ] || [ -L "$TARGET_DIR" ]; then
   echo "Removing existing '$TARGET_DIR'..."
   rm -rf "$TARGET_DIR"
fi

# create the actual symlink: VS Code will now see tools/vscode-aurum
# as if it were installed in its extensions folder
ln -s "$SOURCE_DIR" "$TARGET_DIR"

echo "Symlinked '$SOURCE_DIR' -> '$TARGET_DIR'"
echo "Reload VS Code (Cmd+Shift+P -> 'Developer: Reload Window') to activate."