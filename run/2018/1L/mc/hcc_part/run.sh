#!/bin/bash

# usage: hist-hss-score <categorization-yaml> <lower-bound> <upper-bound> <branch-prefix> <signal-branch-suffix> <signal-category> <dir-to-root-files> [ <more-dir> ... ]
exec ../../../../../bin/hist-hss-score \
    ../categories.yaml \
    0.0 \
    1.0 \
    ak15_ParTMDv2_ \
    Hcc \
    whcc \
    /eos/user/l/legao/hss/samples/Tree/2018/1L/mc/pieces
