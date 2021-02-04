#!/bin/bash

export WORKSPACE=$PWD
docker run \
        -it \
        --rm \
        --privileged \
        --device /dev/ttyUSB0:/dev/ttyUSB0 \
        -e LC_ALL=C.UTF-8 \
        -e IDF_LIB_PATH=/esp-lib \
        -v $WORKSPACE/esp-idf-lib:/esp-lib \
        -v $WORKSPACE/loincloth:/project \
        -w /project \
        espressif/idf:release-v4.2 idf.py
