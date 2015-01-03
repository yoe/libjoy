#!/bin/bash

mkdir m4
gtkdocize --copy
autoreconf -f -i
