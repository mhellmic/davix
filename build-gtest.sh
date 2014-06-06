#!/bin/bash

cd /usr/src/gtest
cmake .
make
mv libg* /usr/lib/
