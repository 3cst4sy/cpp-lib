#! /bin/sh

if [ true = "$debug" ] ; then
  BIN_DIR=../bin/dbg
else 
  BIN_DIR=../bin/opt
fi

GOLDEN_DIR=data/golden-output
INPUT_DIR=data/input
