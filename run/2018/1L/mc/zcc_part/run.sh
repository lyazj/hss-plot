#!/bin/bash

# usage: hist-hss-score <categorization-yaml> <lower-bound> <upper-bound> <branch-prefix> <signal-branch-suffix> <signal-category> <dir-to-root-files> [ <more-dir> ... ]
exec stdbuf -oL ../../../../../bin/hist-hss-score \
    2018_1L_mc.yaml \
    0.0 \
    1.0 \
    ak15_ParTMDV2_ \
    Hcc \
    wzcc \
    /eos/user/l/legao/hss/samples/Tree/2018/1L/mc/pieces \
    2>&1 | tee run.log
