#!/bin/bash

# for now all we do is copy the targets to the top level bin directory
# Meson runs this script from the build directory even though its not there

target_dest=../../bin/rt/
rsync -t "$MESON_BUILD_ROOT"/src/* "$target_dest"