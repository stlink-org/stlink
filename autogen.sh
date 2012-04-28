#!/bin/sh
if test ! "x$(which libtoolize)" = "x"; then
  echo "Running libtoolize"
  libtoolize --copy --force --automake
else
  if test ! "x$(which gintltoolize)" = "x"; then
    echo "Running glibtoolize"
    glibtoolize --copy --force --automake
  fi
fi
autoreconf --install --force --verbose
