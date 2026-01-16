#!/usr/bin/env sh
set -eu

cc main.c -o main \
  -I"$(brew --prefix raylib)/include" \
  -I"$(brew --prefix cjson)/include" \
  -L"$(brew --prefix raylib)/lib" \
  -L"$(brew --prefix cjson)/lib" \
  -lraylib \
  -lcjson \
  -lcurl \
  -lm \
  -framework OpenGL \
  -framework Cocoa \
  -framework IOKit \
  -framework CoreVideo \
  -g3

