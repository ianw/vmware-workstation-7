#!/bin/sh

TARS="vmblock vmci vmmon vmnet vsock"

for i in $TARS
do
    tar cvf $i.tar $i-only
done