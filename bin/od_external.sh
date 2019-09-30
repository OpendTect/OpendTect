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
$*
