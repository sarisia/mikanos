#!/bin/bash

for a in */Makefile; do
    APP_DIR=$(dirname $a)
    # APP_NAME=$(basename $APP_DIR)
    make -C $APP_DIR $@
done
