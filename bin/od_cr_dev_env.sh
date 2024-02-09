#!/usr/bin/env bash
#________________________________________________________________________
#
# Copyright:    (C) 1995-2024 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# Copies the initial files for a new development environment
#

Help()
{
    echo "Syntax: od_cr_dev_env.sh sdkdir envdir"
    echo "Where sdkdir is the OpendTect SDK installation directory"
    echo "and envdir is the location of the new development environment"
}


while test $# -gt 0; do
   case "$1" in
      -h|--help)
        Help
        exit 0
        ;;
     *)
        break
        ;;
    esac
done

if [ $# -lt 2 ]; then
    echo "Invalid syntax: No directories provided"
    Help
    exit 1
fi

inpdir=$1
outdir=$2

plf=`uname`
if [[ "${plf}" == "Linux" ]]; then
    if [ ! -e "${inpdir}/relinfo/ver.devel_lux64.txt" ]; then
	echo $0: warning: '$inpdir' does not seem to have the full development package installed
    fi
elif [[ "${plf}" == "Darwin" ]]; then
    if [ ! -e "${inpdir}/relinfo/ver.devel_macintel.txt" && ! -e "${inpdir}/relinfo/ver.devel_macarm.txt"]; then
	echo $0: warning: '$inpdir' does not seem to have the full development package installed
    fi
else
    echo Unsupported platform
    exit 1
fi

if [ ! -d "${inpdir}" ]; then
    echo "$0: Cannot find the OpendTect SDK directory at '${inpdir}'"
    exit 1
fi

if [ -d "${outdir}" ]; then
    if [ ! -w "${outdir}" ]; then
	echo "$0: The output folder '${outdir}' is not writable. Check the permissions"
	exit 1
    fi
else
    if ! mkdir -p "${outdir}"; then
	echo "$0: Cannot create the output folder '${outdir}'. Check the permissions"
	exit 1
    fi
fi

# ----------------------------------------------------------------------------
# Setup work directory
# ----------------------------------------------------------------------------

if ! cp -a "${inpdir}/doc/Programmer/pluginexample/CMakeLists.txt" "${outdir}" || \
   ! cp -a "${inpdir}/doc/Programmer/pluginexample/plugins" "${outdir}"; then
    echo "$0: Cannot copy all required files"
    exit 1
fi

if [ -e "${outdir}/CMakeCache.txt" ]; then
    exit 0
fi

if ! echo "OpendTect_DIR:PATH=${inpdir}" >> "${outdir}/CMakeCache.txt" || \
   ! echo "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" >> "${outdir}/CMakeCache.txt" ||
   ! echo "CMAKE_CXX_FLAGS_RELWITHDEBINFO=-Wno-inline " >> "${outdir}/CMakeCache.txt" || \
   ! echo "CMAKE_INSTALL_PREFIX=${outdir}/inst" >> "${outdir}/CMakeCache.txt"; then
   echo "$0: warning: failed to initialize the CMakeCache.txt"
fi
if [[ "${plf}" == "Darwin" ]]; then
   if ! echo "AVOID_CLANG_ERROR:BOOL=ON" >> "${outdir}/CMakeCache.txt"; then
       echo "$0: warning: failed to fully initialize the CMakeCache.txt"
   fi
fi
