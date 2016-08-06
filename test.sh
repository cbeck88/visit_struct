#!/bin/bash

set -e

rm -rf bin
rm -rf stage
./run_tests.sh $@
