#!/bin/csh
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# Script copies sources and libs into a development directory
#

if (  $#argv < 2 ) then
    echo "Usage : $0 OpendTect_directory existing_target_directory"
    exit 1
endif

set inpdir="$1"
set outdir="$2"

set cpcmd="cp -a"

if ( ! -e "$inpdir/relinfo/ver.devel_lux64.txt" && ! -e "$inpdir/relinfo/ver.devel_mac.txt") then
     echo "$0: warning: $inpdir does not have the development package installed"
endif
if ( ! -d "$inpdir" ) then
    echo "$0 : $inpdir does not exist"
    exit 1
endif

if ( ! -w "$outdir"  ) then
    echo "$0 : target directory $outdir not writable"
    exit 1
endif

# ----------------------------------------------------------------------------
# Setup work directory
# ----------------------------------------------------------------------------

cd "$outdir"

$cpcmd "${inpdir}/doc/Programmer/pluginexample/CMakeLists.txt" .
$cpcmd "${inpdir}/doc/Programmer/pluginexample/plugins" .
if ( ! -e "${outdir}/plugins" ) then
    mkdir "${outdir}/plugins"
    mkdir "${outdir}/plugins/Tut"
    mkdir "${outdir}/plugins/uiTut"
    cp -a "${inpdir}/plugins/Tut/CMakeLists.txt" "${outdir}/plugins/Tut"
    cp -a ${inpdir}/plugins/Tut/*.h "${outdir}/plugins/Tut"
    cp -a ${inpdir}/plugins/Tut/*.cc "${outdir}/plugins/Tut"
    cp -a "${inpdir}/plugins/uiTut/CMakeLists.txt" "${outdir}/plugins/uiTut"
    cp -a ${inpdir}/plugins/uiTut/*.h "${outdir}/plugins/uiTut"
    cp -a ${inpdir}/plugins/uiTut/*.cc "${outdir}/plugins/uiTut"
endif
chmod +w -R .

echo "OpendTect_DIR:PATH=${inpdir}" >> CMakeCache.txt
echo "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" >> CMakeCache.txt
echo "CMAKE_CXX_FLAGS_RELWITHDEBINFO=-Wno-inline " >> CMakeCache.txt
set plf=`uname`
if ( "${plf}" == "Darwin") then
    echo "AVOID_CLANG_ERROR:BOOL=ON" >> CMakeCache.txt
endif
