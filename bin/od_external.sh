#!/bin/bash
# od_external.sh
# Removes pre-existing OpendTect environment
# if any before executing a command

if [ "$#" -lt 1 ]; then
    echo "Usage: command [ARGS]"
    exit 1	
fi

if [[ -z "${DTECT_APPL}" ]]; then
    plfname=`uname -s`
    if [[ ${plfname} == "Darwin" ]]; then
        execpath="realpath "${0}""
    else
        execpath=`realpath "${0}"`
    fi
    execappl=$(dirname "${execpath}")
    export DTECT_APPL="$(dirname "${execappl}")"
fi

ld_cleanup () {
    local inppaths="${1}"
    local addedpath="${2}"
    if [[ -n "${inppaths}" ]]; then
	if [[ -n "${addedpath}" ]]; then
	    inppaths="${inppaths}":"${addedpath}"
	fi
    elif [[ -n "${addedpath}" ]]; then
	inppaths="${addedpath}"
    fi

    if [[ -z "${inppaths}" ]]; then
	return
    fi

    local func_result=""
    IFS=':' read -r -a ldpaths <<< "${inppaths}"
    for index in "${!ldpaths[@]}"
    do
	if [[ "${ldpaths[index]}" != "${DTECT_APPL}"* ]] &&
	   [[ ! -e "${ldpaths[index]}/libBasic.so" ]] &&
	   ! compgen -G "${ldpaths[index]}/libQt*Core.so*" > /dev/null; then
	    if [[ -n "${func_result}" ]]; then
		export func_result="${func_result}":"${ldpaths[index]}"
	    else
		export func_result="${ldpaths[index]}"
	    fi	    
	fi
    done
    echo "${func_result}"
}

export PATH="$(ld_cleanup "${PATH}" "${OD_INTERNAL_CLEANPATH}")"
export LD_LIBRARY_PATH="$(ld_cleanup "${LD_LIBRARY_PATH}" "${OD_SYSTEM_LIBRARY_PATH}")"
if [[ -z "${LD_LIBRARY_PATH}" ]]; then unset LD_LIBRARY_PATH; fi
unset DTECT_APPL
unset LD_BIND_NOW

#Tensorboard won't start if LC_ALL is not set
if [[ -z $LC_ALL ]] && [[ -n ${LANG} ]]; then
    export LC_ALL=${LANG}
fi

"$@"
