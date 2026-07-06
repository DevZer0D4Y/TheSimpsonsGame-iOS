#!/bin/bash
# The Simpsons Game -- Recompiled : launcher entry point
cd "$(dirname "$(readlink -f "$0")")"
exec python3 launcher.py "$@"
