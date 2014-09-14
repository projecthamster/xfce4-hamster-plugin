#!/bin/bash
autoreconf -sif
intltool-prepare
./configure --enable-silent-rules --silent $*
./config.status -V
echo "Preparation done"
