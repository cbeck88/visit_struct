#!/bin/bash

set -e

if hash b2 2>/dev/null; then
  b2 "$@"
elif hash bjam 2>/dev/null; then
  bjam "$@"
else
  echo >&2 "Require b2 or bjam but it was not found. Aborting."
  exit 1
fi

for file in stage/*
do
  echo ${file} "..."
  gdb -return-child-result -batch -ex "run" -ex "thread apply all bt" -ex "quit" --args ./${file}
done
