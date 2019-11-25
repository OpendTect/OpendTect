#!/bin/bash

if [ "$#" -lt 1 ]; then
    echo "Usage: command [ARGS]"
    exit 1	
fi

if [[ ! -z "${OD_INTERNAL_CLEANPATH}" ]]; then
    export PATH=${OD_INTERNAL_CLEANPATH}
fi
if [[ -z "${OD_SYSTEM_LIBRARY_PATH}" ]]; then
    unset LD_LIBRARY_PATH
else
    export LD_LIBRARY_PATH=${OD_SYSTEM_LIBRARY_PATH}
fi

unset DTECT_APPL

#Tensorboard won't start if LC_ALL is not set
if [ -z $LC_ALL ] && [ ! -z ${LANG} ]; then
    export LC_ALL=${LANG}
fi

$*
