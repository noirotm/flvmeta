#!/bin/sh
if aclocal && autoheader && automake -a && autoconf; then
  echo "Next, run ./configure && make"
else
  echo
  echo "An error occured."
  exit 1
fi
