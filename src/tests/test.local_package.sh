#!/bin/sh

ROOT="test_root"

mkdir -p $ROOT

../tulpar local bash* --root $ROOT 
#ls -R $ROOT 
